#include "Data.h"
#include <conio.h>

void DescribeState(const entt::registry& state) {
    auto& defs = state.ctx<Defs>();

    printf("There's ");
    auto persons = state.view<const Person>();
    for (auto it = persons.begin(); it != persons.end(); ++it) {
        const auto& person = state.get<Person>(*it);
        const auto& personDef = defs.persons[person.defId.value];

        if (it != persons.begin())
        {
            printf(", ");
        }
        printf("%s", personDef.name.c_str());
    }

    printf(". ");
}

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

template<typename VisitorT>
void VisitActionsForPerson(entt::registry& state, entt::entity person, VisitorT& visitor) {
    auto& defs = state.ctx<Defs>();

    for (const auto actionIdx : defs.personActions) {
        const auto& actionDef = defs.actions[actionIdx.value];

        switch (actionDef.targetType) {
        case ActionTargetType::None:
            visitor(actionDef);
            break;

        case ActionTargetType::Person:
            for (const auto targetPerson : state.view<Person>()) {
                TargetPerson target;
                target.person = targetPerson;
                visitor(actionDef, target);
            }
            break;

        case ActionTargetType::ItemOnPerson:
            for (const auto targetEnt : state.view<Person>()) {
                auto& targetInventory = state.get<Inventory>(targetEnt);

                for (const auto targetItem : targetInventory.items) {
                    TargetItemOnPerson target;
                    target.person = targetEnt;
                    target.item = targetItem;
                    visitor(actionDef, target);
                }
            }
            break;

        default:
            assert(!"fixme");
        }
    }

    auto& inventory = state.get<Inventory>(person);
    for (const auto itemEntity : inventory.items) {
        const auto& item = state.get<Item>(itemEntity);
        const auto& itemDef = defs.items[item.defId.value];

        for (const auto actionIdx : itemDef.possibleActions) {
            const auto& actionDef = defs.actions[actionIdx.value];

            switch (actionDef.targetType) {
            case ActionTargetType::Person:
                for (const auto targetEntity : state.view<Person>()) {
                    TargetPersonWithItem target;
                    target.person = targetEntity;
                    target.item = itemEntity;
                    visitor(actionDef, target);
                }
                break;

            default:
                assert(!"fixme");
            }
        }
    }
}

void ExecuteActionForPerson(entt::registry& state, entt::entity person, size_t actionIdx)
{
    struct Visitor {
        size_t m_idx = 0;
        size_t m_executeIdx;
        entt::registry& m_state;

        Visitor(entt::registry& state, size_t executeIdx) : m_state(state), m_executeIdx(executeIdx) {}

        void operator()(const ActionDef& actionDef) {
            if (m_idx == m_executeIdx) {
                auto ctx = ActionContext{ m_state };
                actionDef.apply(ctx);
            }

            ++m_idx;
        }

        void operator()(const ActionDef& actionDef, const TargetPerson& target) {
            if (m_idx == m_executeIdx) {
                auto ctx = ActionContext{ m_state };
                ctx.targetEntity = targetEntity;
                actionDef.apply(ctx);
            }

            ++m_idx;
        }

        void operator()(const ActionDef& actionDef, const TargetPersonWithItem& target) {
            if (m_idx == m_executeIdx) {
                auto ctx = ActionContext{ m_state };
                ctx.targetEntity = targetEntity;
                actionDef.apply(ctx);
            }

            ++m_idx;
        }

        void operator()(const ActionDef& actionDef, const TargetItemOnPerson& target) {
            if (m_idx == m_executeIdx) {
                auto ctx = ActionContext{ m_state };
                ctx.targetEntity = targetEntity;
                actionDef.apply(ctx);
            }

            ++m_idx;
        }
    } visitor(state, actionIdx);

    VisitActionsForPerson(state, person, visitor);
}

class Game {
public:
    Game() {
        m_state.set<Defs>();
    }

    void run() {
        DescribeState(m_state);
        printf("\n\n");

        while (true) {
            PrintPlayerActions();

            printf("Action: ");
            auto c = _getch();
            if (c == 27)
                break;

            printf("%d\n", c - 48);

            ExecutePlayerAction(static_cast<size_t>(c - 49));

            printf("\n");
        }
    }

protected:
    entt::registry m_state;

    void PrintPlayerActions() {
        struct Visitor {
            unsigned int m_idx = 0;
            entt::registry& m_state;

            Visitor(entt::registry& state) : m_state(state) {}

            void operator()(const ActionDef& actionDef) {
                printf("%u: %s\n", m_idx + 1, actionDef.name.c_str());
                ++m_idx;
            }

            void operator()(const ActionDef& actionDef, const TargetPerson& target) {
                auto& defs = m_state.ctx<Defs>();

                const auto& targetPerson = m_state.get<Person>(target.person);
                const auto& targetPersonDef = defs.persons[targetPerson.defId.value];
                printf("%u: %s %s\n",
                       m_idx + 1,
                       actionDef.name.c_str(),
                       targetPersonDef.name.c_str());

                ++m_idx;
            }

            void operator()(const ActionDef& actionDef, const TargetPersonWithItem& target) {
                auto& defs = m_state.ctx<Defs>();

                const auto& targetPerson = m_state.get<Person>(target.person);
                const auto& targetPersonDef = defs.persons[targetPerson.defId.value];
				const auto& item = m_state.get<Item>(target.item);
                const auto& itemDef = defs.items[item.defId.value];
                printf("%d: %s %s with %s\n",
                       m_idx + 1,
                       actionDef.name.c_str(),
                       targetPersonDef.name.c_str(),
                       itemDef.name.c_str());

                ++m_idx;
            }

            void operator()(const ActionDef& actionDef, const TargetItem& target) {
                auto& defs = m_state.ctx<Defs>();

                const auto& targetPerson = m_state.get<Person>(target.person);
                const auto& targetPersonDef = defs.persons[targetPerson.defId.value];
				const auto& targetItem = m_state.get<Item>(target.item);
				const auto& targetItemDef = defs.items[targetItem.defId.value];
                printf("%d: %s %s's %s\n",
                       m_idx + 1,
                       actionDef.name.c_str(),
                       targetPersonDef.name.c_str(),
                       targetItemDef.name.c_str());

                ++m_idx;
            }
        } visitor(m_state);

        // Assume first person is player
        auto player = m_state.view<Person>().front();
        VisitActionsForPerson(m_state, player, visitor);
    }

    void ExecutePlayerAction(size_t actionIdx) {
        // Assume first person is player
        auto player = m_state.view<Person>().front();
        ExecuteActionForPerson(m_state, player, actionIdx);
    }

    ActionDefId CreateActionDef(std::string name, ActionTargetType targetType, std::function<void(const ActionContext&)> apply) {
        auto& defs = m_state.ctx<Defs>();

        const auto defIdx = defs.actions.size();

        ActionDef def;
        def.name = name;
        def.targetType = targetType;
        def.apply = apply;
        defs.actions.push_back(def);

        return ActionDefId{ defIdx };
    }

    void AddPersonAction(ActionDefId action) {
        auto& defs = m_state.ctx<Defs>();
        defs.personActions.push_back(action);
    }

    ItemDefId CreateItemDef(std::string name, std::vector<ActionDefId> possibleActions) {
        auto& defs = m_state.ctx<Defs>();

        const auto defIdx = defs.items.size();

        ItemDef def;
        def.name = name;
        def.possibleActions = possibleActions;
        defs.items.push_back(def);

        return ItemDefId{ defIdx };
    }

    entt::entity CreateItem(ItemDefId defId) {
        auto entity = m_state.create();
        
        m_state.emplace<Item>(entity, defId);

        return entity;
    }

    void AddItem(entt::entity person, ItemDefId defId) {
        auto item = CreateItem(defId);

        auto& inventory = m_state.get<Inventory>(person);
        inventory.items.push_back(item);
    }

    entt::entity CreatePerson(std::string name) {
        auto& defs = m_state.ctx<Defs>();

        const auto defIdx = defs.persons.size();
        PersonDef def;
        def.name = name;
        defs.persons.push_back(def);

        auto p = m_state.create();
        m_state.emplace<Person>(p, PersonDefId{ defIdx });
        m_state.emplace<Inventory>(p);

        return p;
    }
};




class TestGame : public Game {
public:
    TestGame() {
        const auto idle = CreateActionDef("Sleep", ActionTargetType::None, [](const ActionContext& ctx) {});
        const auto steal = CreateActionDef("Steal", ActionTargetType::ItemOnPerson, [](const ActionContext& ctx) {
            printf("Stealing...\n");
        });
        const auto hit = CreateActionDef("Hit", ActionTargetType::Person, [](const ActionContext& ctx) {
            const auto& defs = ctx.state.ctx<Defs>();

            const auto& person = ctx.state.get<Person>(ctx.targetEntity);
            const auto& def = defs.persons[person.defId.value];

            printf("Hitting %s...\n", def.name.c_str());
        });
        AddPersonAction(idle);
        AddPersonAction(steal);
        AddPersonAction(hit);

        const auto stab = CreateActionDef("Stab", ActionTargetType::Person, [](auto& ctx) {});
        const auto knife = CreateItemDef("Knife", { stab });

        auto player = CreatePerson("Player");
        AddItem(player, knife);

        auto king = CreatePerson("King");

        const auto boomerang = CreateItemDef("Boomerang", {});
        const auto darts = CreateItemDef("Darts", {});

        auto chloe = CreatePerson("Chloe");
        AddItem(chloe, boomerang);
        AddItem(chloe, darts);
    }
};

int main(int argc, char** argv) {
    TestGame game;
    game.run();

    return 0;
}