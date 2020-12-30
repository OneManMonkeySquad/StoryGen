#pragma once

#include "entt/entt.hpp"
#include <string>
#include <vector>

class Game;

struct ActionDefId { size_t value; };
struct ItemDefId { size_t value; };
struct PersonDefId { size_t value; };

enum class ActionTargetType {
    None,
    Person,
    ItemOnGround,
    ItemOnPerson
};

struct ActionContext {
    Game& g;
    entt::entity actor;
    entt::entity targetEntity = entt::null;
    entt::entity targetEntity2 = entt::null;
};

struct ActionDef {
    std::string name;
    ActionTargetType targetType;
    std::function<void(const ActionContext&)> apply;
};

struct TargetPerson {
	entt::entity person;
};

struct TargetPersonWithItem {
	entt::entity person;
	entt::entity item;
};

struct TargetItemOnPerson {
	entt::entity person;
	entt::entity item;
};

struct ItemDef {
    std::string name;
    std::vector<ActionDefId> possibleActions;
};

struct PersonDef {
    std::string name;
};

struct Defs {
    std::vector<ItemDef> items;
    std::vector<ActionDef> actions;
    std::vector<PersonDef> persons;

    std::vector<ActionDefId> personActions;
};

namespace C {
	struct Item {
		ItemDefId defId;
	};

	struct Inventory {
		std::vector<entt::entity> items;
	};

	struct Person {
		PersonDefId defId;
	};

    struct Dead {
    };

    struct Player {
    };
}
