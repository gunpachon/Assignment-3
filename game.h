#ifndef GAME_H
#define GAME_H

#include <memory>
#include <string>
#include <vector>

class Room; 

class GameObject {
public:
    virtual ~GameObject();
    GameObject(const GameObject&) = delete;             // objects are unique: no copies
    GameObject& operator=(const GameObject&) = delete;
    const std::string& name() const { return name_; }
    virtual std::string describe() const = 0;            // abstract
    static int count() { return count_; }                 // objects alive right now
protected:
    explicit GameObject(std::string name);                // only subclasses construct
private:
    std::string name_;
    static int count_;                                    // class (static) variable
};

// ---------- Things ----------
class Item : public GameObject {
public:
    Item(std::string name, int weight);
    virtual int weight() const { return weight_; }
    virtual bool canBeCarried() const { return true; }
    std::string describe() const override { return name(); }
private:
    int weight_;
};

class Weapon : public Item {
public:
    Weapon(std::string name, int weight, int damage);
    int damage() const { return damage_; }
    std::string describe() const override;
private:
    int damage_;
};

class Treasure : public Item {
public:
    Treasure(std::string name, int weight, int value);
    int value() const { return value_; }
    std::string describe() const override;
private:
    int value_;
};

// ---------- Inventory: a list of items, reused by Container, Creature and Room ----------
class Inventory {
public:
    void add(std::unique_ptr<Item> item);
    std::unique_ptr<Item> take(const std::string& name);  // remove and return, or nullptr
    Item* find(const std::string& name) const;            // look without removing
    void moveAllTo(Inventory& other);
    int totalWeight() const;
    std::string list() const;
    bool empty() const { return items_.empty(); }
private:
    std::vector<std::unique_ptr<Item>> items_;
};

class Container : public Item {
public:
    Container(std::string name, int weight);
    virtual bool put(std::unique_ptr<Item>& item);        // false: caller keeps the item
    std::unique_ptr<Item> take(const std::string& name) { return contents_.take(name); }
    int weight() const override { return Item::weight() + contents_.totalWeight(); }
    std::string describe() const override;
protected:
    Inventory contents_;
};

class Bag : public Container {
public:
    Bag(std::string name, int maxWeight);
    bool put(std::unique_ptr<Item>& item) override;       // refuses if too heavy
private:
    int maxWeight_;
};

class Chest : public Container {
public:
    Chest(std::string name, bool locked);
    bool put(std::unique_ptr<Item>& item) override;       // refuses if locked
    void lock() { locked_ = true; }
    void unlock() { locked_ = false; }
    bool canBeCarried() const override { return false; }
    std::string describe() const override;
private:
    bool locked_;
};

// ---------- Characters ----------
class Creature : public GameObject {
public:
    void attack(Creature& target);                        // same for all, uses attackPower()
    virtual void takeDamage(int amount);
    bool isDefeated() const { return hp_ == 0; }
    int hitPoints() const { return hp_; }
    int maxHitPoints() const { return maxHp_; }
    bool pickUp(std::unique_ptr<Item>& item);             // false: caller keeps the item
    virtual std::unique_ptr<Item> drop(const std::string& name);
    Inventory& inventory() { return inventory_; }
    std::string describe() const override;
protected:
    Creature(std::string name, int maxHp);
    virtual int attackPower(const Creature& target) = 0;  // the polymorphic part
    void heal(int amount);
private:
    int hp_, maxHp_;                                      // only changed via takeDamage/heal
    Inventory inventory_;
};

class Player : public Creature {
public:
    Player(std::string name, int maxHp);
    bool wield(const std::string& weaponName);
    std::unique_ptr<Item> drop(const std::string& name) override;
protected:
    int attackPower(const Creature& target) override;
private:
    Weapon* wielded_ = nullptr;                           // non-owning: inventory owns it
};

class Monster : public Creature {
public:
    void act(Room& room);                                 // the program's "AI"
protected:
    Monster(std::string name, int maxHp);
};

class Dragon : public Monster {
public:
    explicit Dragon(std::string name);
protected:
    int attackPower(const Creature& target) override;
private:
    int attacks_ = 0;
};

class Troll : public Monster {
public:
    explicit Troll(std::string name);
    void takeDamage(int amount) override;                 // regenerates after a hit
protected:
    int attackPower(const Creature& target) override;
};

class Shark : public Monster {
public:
    explicit Shark(std::string name);
protected:
    int attackPower(const Creature& target) override;
};

// ---------- Places ----------
class Room : public GameObject {
public:
    explicit Room(std::string name);
    void add(std::unique_ptr<Creature> creature);
    void add(std::unique_ptr<Item> item) { items_.add(std::move(item)); }
    Creature* findCreature(const std::string& name) const;
    Player* firstPlayer() const;
    void removeDefeated();                                // their items fall on the floor
    void monstersAct();                                   // every monster takes its turn
    int monstersLeft() const;
    Inventory& items() { return items_; }
    std::string describe() const override;
private:
    Inventory items_;
    std::vector<std::unique_ptr<Creature>> creatures_;
};

#endif
