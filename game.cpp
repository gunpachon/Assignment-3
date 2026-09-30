// game.cpp - Implementation of the classes in game.h
#include "game.h"
#include <algorithm>
#include <iostream>

// ---------- GameObject ----------
int GameObject::count_ = 0;

GameObject::GameObject(std::string name) : name_(std::move(name)) { ++count_; }
GameObject::~GameObject() { --count_; }

// ---------- Items ----------
Item::Item(std::string name, int weight) : GameObject(std::move(name)), weight_(weight) {}

Weapon::Weapon(std::string name, int weight, int damage)
    : Item(std::move(name), weight), damage_(damage) {}

std::string Weapon::describe() const {
    return name() + " (damage " + std::to_string(damage_) + ")";
}

Treasure::Treasure(std::string name, int weight, int value)
    : Item(std::move(name), weight), value_(value) {}

std::string Treasure::describe() const {
    return name() + " (worth " + std::to_string(value_) + ")";
}

// ---------- Inventory ----------
void Inventory::add(std::unique_ptr<Item> item) {
    if (item) items_.push_back(std::move(item));
}

std::unique_ptr<Item> Inventory::take(const std::string& name) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if ((*it)->name() == name) {
            std::unique_ptr<Item> item = std::move(*it);
            items_.erase(it);
            return item;
        }
    }
    return nullptr;
}

Item* Inventory::find(const std::string& name) const {
    for (const auto& item : items_)
        if (item->name() == name) return item.get();
    return nullptr;
}

void Inventory::moveAllTo(Inventory& other) {
    for (auto& item : items_) other.add(std::move(item));
    items_.clear();
}

int Inventory::totalWeight() const {
    int sum = 0;
    for (const auto& item : items_) sum += item->weight();  // virtual: bags count contents
    return sum;
}

std::string Inventory::list() const {
    if (items_.empty()) return "nothing";
    std::string result;
    for (const auto& item : items_) {
        if (!result.empty()) result += ", ";
        result += item->describe();                          // virtual: each item describes itself
    }
    return result;
}

// ---------- Containers ----------
Container::Container(std::string name, int weight) : Item(std::move(name), weight) {}

bool Container::put(std::unique_ptr<Item>& item) {
    if (!item || item.get() == this) return false;           // can't put a bag in itself
    contents_.add(std::move(item));
    return true;
}

std::string Container::describe() const {
    return name() + " [containing: " + contents_.list() + "]";
}

Bag::Bag(std::string name, int maxWeight) : Container(std::move(name), 1), maxWeight_(maxWeight) {}

bool Bag::put(std::unique_ptr<Item>& item) {
    if (!item || contents_.totalWeight() + item->weight() > maxWeight_) return false;
    return Container::put(item);
}

Chest::Chest(std::string name, bool locked) : Container(std::move(name), 40), locked_(locked) {}

bool Chest::put(std::unique_ptr<Item>& item) {
    if (locked_) return false;
    return Container::put(item);
}

std::string Chest::describe() const {
    return Container::describe() + (locked_ ? " (locked)" : "");
}

// ---------- Creature ----------
Creature::Creature(std::string name, int maxHp)
    : GameObject(std::move(name)), hp_(maxHp), maxHp_(maxHp) {}

void Creature::attack(Creature& target) {
    if (isDefeated() || target.isDefeated()) return;
    int damage = attackPower(target);                         // dynamic method lookup
    std::cout << "  " << name() << " attacks " << target.name()
              << " for " << damage << " damage.\n";
    target.takeDamage(damage);                                // also virtual
    std::cout << "    " << target.name() << " has " << target.hitPoints() << "/"
              << target.maxHitPoints() << " HP"
              << (target.isDefeated() ? " and is DEFEATED!" : ".") << "\n";
}

void Creature::takeDamage(int amount) { hp_ = std::max(0, hp_ - amount); }

void Creature::heal(int amount) {
    if (!isDefeated()) hp_ = std::min(maxHp_, hp_ + amount);
}

bool Creature::pickUp(std::unique_ptr<Item>& item) {
    if (!item || !item->canBeCarried()) return false;
    inventory_.add(std::move(item));
    return true;
}

std::unique_ptr<Item> Creature::drop(const std::string& name) { return inventory_.take(name); }

std::string Creature::describe() const {
    return name() + " (" + std::to_string(hp_) + "/" + std::to_string(maxHp_) +
           " HP, carrying: " + inventory_.list() + ")";
}

// ---------- Player ----------
Player::Player(std::string name, int maxHp) : Creature(std::move(name), maxHp) {}

bool Player::wield(const std::string& weaponName) {
    auto* weapon = dynamic_cast<Weapon*>(inventory().find(weaponName));
    if (!weapon) return false;                                // not carried, or not a weapon
    wielded_ = weapon;
    return true;
}

std::unique_ptr<Item> Player::drop(const std::string& name) {
    std::unique_ptr<Item> item = Creature::drop(name);
    if (item && item.get() == wielded_) wielded_ = nullptr;   // avoid a dangling pointer
    return item;
}

int Player::attackPower(const Creature&) { return wielded_ ? wielded_->damage() : 2; }

// ---------- Monsters ----------
Monster::Monster(std::string name, int maxHp) : Creature(std::move(name), maxHp) {}

void Monster::act(Room& room) {
    Player* player = room.firstPlayer();
    if (player) attack(*player);                              // attack() checks isDefeated
}

Dragon::Dragon(std::string name) : Monster(std::move(name), 60) {}

int Dragon::attackPower(const Creature&) {
    if (++attacks_ % 3 == 0) {
        std::cout << "  " << name() << " breathes FIRE!\n";
        return 25;
    }
    return 10;
}

Troll::Troll(std::string name) : Monster(std::move(name), 20) {}

void Troll::takeDamage(int amount) {
    Creature::takeDamage(amount);                             // reuse the base behaviour
    if (!isDefeated()) {
        heal(3);
        std::cout << "    " << name() << " regenerates 3 HP.\n";
    }
}

int Troll::attackPower(const Creature&) { return 8; }

Shark::Shark(std::string name) : Monster(std::move(name), 25) {}

int Shark::attackPower(const Creature& target) {
    if (target.hitPoints() < target.maxHitPoints() / 2) {
        std::cout << "  " << name() << " smells blood!\n";
        return 12;
    }
    return 6;
}

// ---------- Room ----------
Room::Room(std::string name) : GameObject(std::move(name)) {}

void Room::add(std::unique_ptr<Creature> creature) {
    if (creature) creatures_.push_back(std::move(creature));
}

Creature* Room::findCreature(const std::string& name) const {
    for (const auto& c : creatures_)
        if (c->name() == name) return c.get();
    return nullptr;
}

Player* Room::firstPlayer() const {
    for (const auto& c : creatures_)
        if (auto* p = dynamic_cast<Player*>(c.get()); p && !p->isDefeated()) return p;
    return nullptr;
}

void Room::removeDefeated() {
    for (auto it = creatures_.begin(); it != creatures_.end();) {
        if ((*it)->isDefeated()) {
            std::cout << "  " << (*it)->name() << " is removed and drops: "
                      << (*it)->inventory().list() << "\n";
            (*it)->inventory().moveAllTo(items_);
            it = creatures_.erase(it);                        // unique_ptr deletes it
        } else {
            ++it;
        }
    }
}

void Room::monstersAct() {
    for (const auto& c : creatures_)
        if (auto* m = dynamic_cast<Monster*>(c.get())) m->act(*this);
}

int Room::monstersLeft() const {
    int n = 0;
    for (const auto& c : creatures_)
        if (dynamic_cast<Monster*>(c.get()) && !c->isDefeated()) ++n;
    return n;
}

std::string Room::describe() const {
    std::string result = "== " + name() + " ==\n  Creatures:";
    for (const auto& c : creatures_) result += "\n    " + c->describe();
    return result + "\n  Items on the floor: " + items_.list();
}
