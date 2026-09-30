// main.cpp - Play the game (./game) or watch a scripted demo (./game demo)
#include "game.h"
#include <iostream>
#include <sstream>

// Create an object, hand ownership to the room, and keep a plain pointer to it.
template <class T, class... Args>
T* spawn(Room& room, Args&&... args) {
    auto object = std::make_unique<T>(std::forward<Args>(args)...);
    T* pointer = object.get();
    room.add(std::move(object));
    return pointer;
}

void step(const std::string& text) { std::cout << "\n> " << text << "\n"; }

void runDemo() {
    Room cave("cave");
    Player* gun = spawn<Player>(cave, "Gun", 50);
    Troll* troll = spawn<Troll>(cave, "troll");
    Shark* shark = spawn<Shark>(cave, "shark");
    spawn<Weapon>(cave, "sword", 3, 9);
    Bag* bag = spawn<Bag>(cave, "bag", 5);
    spawn<Treasure>(cave, "coins", 2, 100);
    Chest* chest = spawn<Chest>(cave, "chest", true);
    std::unique_ptr<Item> club = std::make_unique<Weapon>("club", 5, 8);
    troll->pickUp(club);
    std::cout << cave.describe() << "\nObjects in the world: " << GameObject::count() << "\n";

    step("take chest");
    auto item = cave.items().take("chest");
    if (!gun->pickUp(item)) {
        std::cout << "  The chest is too heavy to carry.\n";
        cave.add(std::move(item));                    // we still own it: put it back
    }

    step("take sword, wield sword, take bag, take coins");
    for (auto name : {"sword", "bag", "coins"}) {
        item = cave.items().take(name);
        gun->pickUp(item);
    }
    gun->wield("sword");
    std::cout << "  " << gun->describe() << "\n";

    step("put coins in bag");
    item = gun->drop("coins");
    bag->put(item);
    std::cout << "  The bag now weighs " << bag->weight() << ".\n";

    step("put bag in chest");
    item = gun->drop("bag");
    if (!chest->put(item)) std::cout << "  The chest is locked.\n";
    step("unlock chest, put bag in chest");
    chest->unlock();
    chest->put(item);
    std::cout << "  " << chest->describe() << "\n";

    step("attack troll (the monsters fight back)");
    while (!troll->isDefeated() && !gun->isDefeated()) {
        gun->attack(*troll);
        troll->act(cave);
        shark->act(cave);
    }

    step("clean up defeated creatures");
    cave.removeDefeated();
    std::cout << "Objects in the world: " << GameObject::count() << "\n";

    step("a dragon flies in and attacks the shark");
    Dragon* dragon = spawn<Dragon>(cave, "dragon");
    for (int i = 0; i < 3; ++i) dragon->attack(*shark);
    cave.removeDefeated();

    std::cout << "\n" << cave.describe() << "\nObjects in the world: "
              << GameObject::count() << "\n";
}

// ---------- Interactive game ----------

// A container can be in the player's hands or on the floor.
Container* findContainer(Room& room, Player& player, const std::string& name) {
    if (auto* c = dynamic_cast<Container*>(player.inventory().find(name))) return c;
    return dynamic_cast<Container*>(room.items().find(name));
}

void printHelp() {
    std::cout << "Commands:\n"
                 "  look                 describe the room\n"
                 "  inventory            what you carry (also: i)\n"
                 "  take X               pick up X from the floor\n"
                 "  take X from Y        take X out of bag/chest Y\n"
                 "  drop X               drop X on the floor\n"
                 "  wield X              hold weapon X\n"
                 "  put X in Y           put X into bag/chest Y\n"
                 "  unlock Y             unlock chest Y (you need a key)\n"
                 "  attack X             attack creature X\n"
                 "  help, quit\n"
                 "Monsters take their turn after every action.\n";
}

// Carries out one command. Returns true if it used up the player's turn.
bool handle(const std::vector<std::string>& w, Room& room, Player& me) {
    const std::string& cmd = w[0];
    if (cmd == "look") { std::cout << room.describe() << "\n"; return false; }
    if (cmd == "inventory" || cmd == "i") { std::cout << me.describe() << "\n"; return false; }
    if (cmd == "help") { printHelp(); return false; }

    if (cmd == "take" && w.size() == 2) {
        auto item = room.items().take(w[1]);
        if (!item) { std::cout << "There is no " << w[1] << " here.\n"; return false; }
        if (!me.pickUp(item)) {
            std::cout << "The " << w[1] << " is too heavy to carry.\n";
            room.add(std::move(item));                    // still ours: put it back
            return false;
        }
        std::cout << "You take the " << w[1] << ".\n";
        return true;
    }
    if (cmd == "take" && w.size() == 4 && w[2] == "from") {
        Container* box = findContainer(room, me, w[3]);
        if (!box) { std::cout << "There is no " << w[3] << " to take from.\n"; return false; }
        auto item = box->take(w[1]);
        if (!item) { std::cout << "The " << w[3] << " has no " << w[1] << ".\n"; return false; }
        me.pickUp(item);
        std::cout << "You take the " << w[1] << " from the " << w[3] << ".\n";
        return true;
    }
    if (cmd == "drop" && w.size() == 2) {
        auto item = me.drop(w[1]);
        if (!item) { std::cout << "You don't carry a " << w[1] << ".\n"; return false; }
        room.add(std::move(item));
        std::cout << "You drop the " << w[1] << ".\n";
        return true;
    }
    if (cmd == "wield" && w.size() == 2) {
        if (!me.wield(w[1])) { std::cout << "You carry no weapon called " << w[1] << ".\n"; return false; }
        std::cout << "You wield the " << w[1] << ".\n";
        return true;
    }
    if (cmd == "put" && w.size() == 4 && w[2] == "in") {
        Container* box = findContainer(room, me, w[3]);
        if (!box) { std::cout << "There is no " << w[3] << " to put things in.\n"; return false; }
        auto item = me.drop(w[1]);
        if (!item) { std::cout << "You don't carry a " << w[1] << ".\n"; return false; }
        if (!box->put(item)) {                            // Bag: too heavy, Chest: locked
            std::cout << "You can't put the " << w[1] << " in the " << w[3] << ".\n";
            me.pickUp(item);                              // give it back
            return false;
        }
        std::cout << "You put the " << w[1] << " in the " << w[3] << ".\n";
        return true;
    }
    if (cmd == "unlock" && w.size() == 2) {
        auto* chest = dynamic_cast<Chest*>(findContainer(room, me, w[1]));
        if (!chest) { std::cout << "There is no chest called " << w[1] << ".\n"; return false; }
        if (!me.inventory().find("key")) { std::cout << "You need a key.\n"; return false; }
        chest->unlock();
        std::cout << "Click! The " << w[1] << " is unlocked.\n";
        return true;
    }
    if (cmd == "attack" && w.size() == 2) {
        Creature* target = room.findCreature(w[1]);
        if (!target || target == &me) { std::cout << "There is no " << w[1] << " to attack.\n"; return false; }
        me.attack(*target);
        return true;
    }
    std::cout << "I don't understand. Type 'help'.\n";
    return false;
}

void play() {
    Room cave("cave");
    Player* me = spawn<Player>(cave, "Gun", 120);
    Troll* troll = spawn<Troll>(cave, "troll");
    spawn<Shark>(cave, "shark");
    spawn<Weapon>(cave, "sword", 3, 9);
    spawn<Bag>(cave, "bag", 5);
    spawn<Treasure>(cave, "coins", 2, 100);
    Chest* chest = spawn<Chest>(cave, "chest", false);
    std::unique_ptr<Item> crown = std::make_unique<Treasure>("crown", 3, 500);
    chest->put(crown);                                    // hide a crown, then lock it
    chest->lock();
    std::unique_ptr<Item> club = std::make_unique<Weapon>("club", 5, 8);
    std::unique_ptr<Item> key = std::make_unique<Item>("key", 1);
    troll->pickUp(club);
    troll->pickUp(key);

    std::cout << "You wake up in a dark cave. Defeat the monsters!\n";
    printHelp();
    std::cout << "\n" << cave.describe() << "\n";

    std::string line;
    while (std::cout << "\n> " && std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::vector<std::string> words;
        for (std::string word; in >> word;) words.push_back(word);
        if (words.empty()) continue;
        if (words[0] == "quit") break;

        if (!handle(words, cave, *me)) continue;          // no turn used: monsters wait
        cave.monstersAct();
        if (me->isDefeated()) { std::cout << "\nYou have been defeated. GAME OVER.\n"; return; }
        cave.removeDefeated();
        if (cave.monstersLeft() == 0) {
            std::cout << "\nAll monsters are defeated. YOU WIN!\n" << me->describe() << "\n";
            return;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "demo") runDemo();
    else play();
    return 0;
}
