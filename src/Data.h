#pragma once

#include "entt.hpp"
#include <string>
#include <vector>

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
    entt::registry& state;
    entt::entity targetEntity = entt::null;
};

struct ActionDef {
    std::string name;
    ActionTargetType targetType;
    std::function<void(const ActionContext&)> apply;
};

struct ItemDef {
    std::string name;
    std::vector<ActionDefId> possibleActions;
};

struct Item {
    ItemDefId defId;
};

struct PersonDef {
    std::string name;
};

struct Inventory {
    std::vector<entt::entity> items;
};

struct Person {
    PersonDefId defId;
};

struct Defs {
    std::vector<ItemDef> items;
    std::vector<ActionDef> actions;
    std::vector<PersonDef> persons;
    std::vector<ActionDefId> personActions;
};
