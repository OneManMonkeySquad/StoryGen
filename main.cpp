#include "entt.hpp"
#include <string>
#include <conio.h>
#include <assert.h>
#include <vector>
#include <memory>
#include <functional>

enum class ActionTargetType {
	None,
	Person,
	ItemOnGround,
	ItemOnPerson
};

struct ActionContext {
	entt::registry& state;
	entt::entity targetEntity;
};

struct ActionDef {
	std::string name;
	ActionTargetType targetType;
	std::function<void(const ActionContext&)> apply;
};

struct ItemDef {
	std::string name;
	std::vector<uint32_t> possibleActions;
};

struct PersonDef {
	std::string name;
};

struct Inventory {
	std::vector<uint32_t> items;
};

struct Person {
	uint32_t defIdx;
};

struct Defs {
	std::vector<ItemDef> items;
	std::vector<ActionDef> actions;
	std::vector<PersonDef> persons;
	std::vector<uint32_t> personActions;
};

class Game {
public:
	Game() {
		m_state2.set<Defs>();
	}

	void run() {
		DescribeState();
		printf("\n\n");

		while (true) {
			PrintPlayerActions();

			printf("Action: ");
			auto c = _getch();
			if (c == 27)
				break;

			printf("%d\n", c - 48);

			ExecutePlayerAction(c - 49);

			printf("\n");
		}
	}

protected:
	entt::registry m_state2;

	void DescribeState() {
		auto& defs = m_state2.ctx<Defs>();

		printf("There's ");
		for (const auto personEntity : m_state2.view<Person>()) {
			const auto& person = m_state2.get<Person>(personEntity);
			const auto& personDef = defs.persons[person.defIdx];

			printf("%s, ", personDef.name.c_str());
		}

		printf(". ");
	}

	template<typename VisitorT>
	void VisitActionsForPerson(entt::entity person, VisitorT& visitor) {
		auto& defs = m_state2.ctx<Defs>();

		for (const auto actionIdx : defs.personActions) {
			const auto& actionDef = defs.actions[actionIdx];

			switch (actionDef.targetType) {
			case ActionTargetType::None:
				visitor(actionDef);
				break;

			case ActionTargetType::Person:
				for (const auto targetPerson : m_state2.view<Person>()) {
					visitor(actionDef, targetPerson);
				}
				break;

			case ActionTargetType::ItemOnPerson:
				for (const auto targetEntity : m_state2.view<Person>()) {
					auto& targetInventory = m_state2.get<Inventory>(targetEntity);

					for (const auto targetItemIdx : targetInventory.items) {
						const auto& targetItemDef = defs.items[targetItemIdx];
						visitor(actionDef, targetEntity, targetItemDef);
					}
				}
				break;

			default:
				assert(!"fixme");
			}
		}

		auto& inventory = m_state2.get<Inventory>(person);
		for (const auto itemIdx : inventory.items) {
			const auto& itemDef = defs.items[itemIdx];

			for (const auto actionIdx : itemDef.possibleActions) {
				const auto& actionDef = defs.actions[actionIdx];

				switch (actionDef.targetType) {
				case ActionTargetType::Person:
					for (const auto targetEntity : m_state2.view<Person>()) {
						visitor(actionDef, itemDef, targetEntity);
					}
					break;

				default:
					assert(!"fixme");
				}
			}
		}
	}

	void PrintPlayerActions() {
		struct Visitor {
			size_t m_idx = 0;
			entt::registry& m_state;

			Visitor(entt::registry& state) : m_state(state) {}

			void operator()(const ActionDef& actionDef) {
				printf("%d: %s\n", m_idx + 1, actionDef.name.c_str());
				++m_idx;
			}

			void operator()(const ActionDef& actionDef, entt::entity targetEntity) {
				auto& defs = m_state.ctx<Defs>();

				const auto& targetPerson = m_state.get<Person>(targetEntity);
				const auto& targetPersonDef = defs.persons[targetPerson.defIdx];
				printf("%d: %s %s\n", m_idx + 1, actionDef.name.c_str(), targetPersonDef.name.c_str());

				++m_idx;
			}

			void operator()(const ActionDef& actionDef, const ItemDef& itemDef, entt::entity targetEntity) {
				auto& defs = m_state.ctx<Defs>();

				const auto& targetPerson = m_state.get<Person>(targetEntity);
				const auto& targetPersonDef = defs.persons[targetPerson.defIdx];
				printf("%d: %s %s with %s\n", m_idx + 1, actionDef.name.c_str(), targetPersonDef.name.c_str(), itemDef.name.c_str());

				++m_idx;
			}

			void operator()(const ActionDef& actionDef, entt::entity targetEntity, const ItemDef& targetItemDef) {
				auto& defs = m_state.ctx<Defs>();

				const auto& targetPerson = m_state.get<Person>(targetEntity);
				const auto& targetPersonDef = defs.persons[targetPerson.defIdx];
				printf("%d: %s %s's %s\n", m_idx + 1, actionDef.name.c_str(), targetPersonDef.name.c_str(), targetItemDef.name.c_str());

				++m_idx;
			}
		} visitor(m_state2);

		// Assume first person is player
		auto player = m_state2.view<Person>().front();
		VisitActionsForPerson(player, visitor);
	}

	void ExecutePlayerAction(size_t idx) {
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

			void operator()(const ActionDef& actionDef, entt::entity targetEntity) {
				if (m_idx == m_executeIdx) {
					auto ctx = ActionContext{ m_state };
					ctx.targetEntity = targetEntity;
					actionDef.apply(ctx);
				}

				++m_idx;
			}

			void operator()(const ActionDef& actionDef, const ItemDef& itemDef, entt::entity targetEntity) {
				if (m_idx == m_executeIdx) {
					auto ctx = ActionContext{ m_state };
					ctx.targetEntity = targetEntity;
					actionDef.apply(ctx);
				}

				++m_idx;
			}

			void operator()(const ActionDef& actionDef, entt::entity targetEntity, const ItemDef& targetItemDef) {
				if (m_idx == m_executeIdx) {
					auto ctx = ActionContext{ m_state };
					ctx.targetEntity = targetEntity;
					actionDef.apply(ctx);
				}

				++m_idx;
			}
		} visitor(m_state2, idx);

		// Assume first person is player
		auto player = m_state2.view<Person>().front();
		VisitActionsForPerson(player, visitor);
	}

	uint32_t CreateActionDef(std::string name, ActionTargetType targetType, std::function<void(const ActionContext&)> apply) {
		auto& defs = m_state2.ctx<Defs>();

		const auto defIdx = defs.actions.size();

		ActionDef def;
		def.name = name;
		def.targetType = targetType;
		def.apply = apply;
		defs.actions.push_back(def);

		return defIdx;
	}

	void AddPersonAction(uint32_t action) {
		auto& defs = m_state2.ctx<Defs>();
		defs.personActions.push_back(action);
	}

	uint32_t CreateItemDef(std::string name, std::vector<uint32_t> possibleActions) {
		auto& defs = m_state2.ctx<Defs>();

		const auto defIdx = defs.items.size();

		ItemDef def;
		def.name = name;
		def.possibleActions = possibleActions;
		defs.items.push_back(def);

		return defIdx;
	}

	void AddItem(entt::entity person, uint32_t defIdx) {
		auto& inventory = m_state2.get<Inventory>(person);
		inventory.items.push_back(defIdx);
	}

	entt::entity CreatePerson(std::string name) {
		auto& defs = m_state2.ctx<Defs>();

		const auto defIdx = defs.persons.size();
		PersonDef def;
		def.name = name;
		defs.persons.push_back(def);

		auto p = m_state2.create();
		m_state2.emplace<Person>(p, defIdx);
		m_state2.emplace<Inventory>(p);

		return p;
	}
};

class TestGame : public Game {
public:
	TestGame() {
		const auto idle = CreateActionDef("Idle", ActionTargetType::None, [](const ActionContext& ctx) {});
		const auto steal = CreateActionDef("Steal", ActionTargetType::ItemOnPerson, [](const ActionContext& ctx) {
			printf("Stealing...\n");
			});
		const auto hit = CreateActionDef("Hit", ActionTargetType::Person, [](const ActionContext& ctx) {
			const auto& defs = ctx.state.ctx<Defs>();

			const auto& person = ctx.state.get<Person>(ctx.targetEntity);
			const auto& def = defs.persons[person.defIdx];

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

		// Needs to include wife stealing!
	}
};

int main(int argc, char** argv) {
	TestGame game;
	game.run();

	return 0;
}