#pragma once

#include "Data.h"
#include <conio.h>

class Game {
public:
	Game() {
		_defs = std::make_shared<Defs>();
	}

	Game(const Game&) = delete;

	Game(Game&& other) noexcept {
		_state = std::move(other._state);
		_defs = std::move(other._defs);
	}

	Game Clone() {
		Game g;
		g._defs = _defs;
		g._state.assign(_state.data(), _state.data() + _state.size());

#define CLONE_EMPTY(T) {auto view = _state.view<T>(); g._state.insert<T>(view.data(), view.data() + view.size());}
#define CLONE_STATEFUL(T) {auto view = _state.view<T>(); g._state.insert<T>(view.data(), view.data() + view.size(), view.raw(), view.raw() + view.size());}

		CLONE_STATEFUL(C::Item);
		CLONE_STATEFUL(C::Inventory);
		CLONE_STATEFUL(C::Person);
		CLONE_EMPTY(C::Dead);
		CLONE_EMPTY(C::Player);

		assert(g._state.size() == _state.size());
		assert(g._state.size<C::Inventory>() == _state.size<C::Inventory>());
		return g;
	}

	void DescribeState() {
		printf("There's ");
		auto persons = _state.view<const C::Person>();
		for (auto it = persons.begin(); it != persons.end(); ++it) {
			const auto& person = _state.get<C::Person>(*it);
			const auto& personDef = _defs->persons[person.defId.value];
			const auto isDead = _state.has<C::Dead>(*it);

			if (it != persons.begin()) {
				printf(", ");
			}
			printf("%s", personDef.name.c_str());
			if (isDead) {
				printf(" (dead)");
			}
		}

		printf(". ");
	}

	void PrintPlayerActions() {
		struct Visitor {
			unsigned int _idx = 0;
			Game& _g;

			Visitor(Game& g) : _g(g) {}

			void operator()(ActionDefId actionId) {
				const auto& actionDef = _g.GetActionDef(actionId);
				printf("%u: %s\n", _idx + 1, actionDef.name.c_str());
				++_idx;
			}

			void operator()(ActionDefId actionId, const TargetPerson& target) {
				const auto& actionDef = _g.GetActionDef(actionId);
				const auto& targetPersonDef = _g.GetPersonDef(target.person);
				printf("%u: %s %s\n",
					_idx + 1,
					actionDef.name.c_str(),
					targetPersonDef.name.c_str());

				++_idx;
			}

			void operator()(ActionDefId actionId, const TargetPersonWithItem& target) {
				const auto& actionDef = _g.GetActionDef(actionId);
				const auto& targetPersonDef = _g.GetPersonDef(target.person);
				const auto& itemDef = _g.GetItemDef(target.item);
				printf("%d: %s %s with %s\n",
					_idx + 1,
					actionDef.name.c_str(),
					targetPersonDef.name.c_str(),
					itemDef.name.c_str());

				++_idx;
			}

			void operator()(ActionDefId actionId, const TargetItemOnPerson& target) {
				const auto& actionDef = _g.GetActionDef(actionId);
				const auto& targetPersonDef = _g.GetPersonDef(target.person);
				const auto& targetItemDef = _g.GetItemDef(target.item);
				printf("%d: %s %s's %s\n",
					_idx + 1,
					actionDef.name.c_str(),
					targetPersonDef.name.c_str(),
					targetItemDef.name.c_str());

				++_idx;
			}
		} visitor(*this);

		auto player = _state.view<C::Player>().front();
		VisitPossibleActionsForPerson(player, visitor);
	}

	void ExecutePlayerAction(size_t actionIdx) {
		auto player = _state.view<C::Player>().front();
		ExecuteActionForPerson(player, actionIdx);
	}

	void AddItem(entt::entity person, entt::entity item) {
		assert(_state.has<C::Item>(item));

		auto& inventory = _state.get<C::Inventory>(person);
		inventory.items.push_back(item);
	}

	bool RemoveItem(entt::entity person, entt::entity item) {
		assert(_state.has<C::Item>(item));

		auto& inventory = _state.get<C::Inventory>(person);
		auto& items = inventory.items;

		auto it = std::find(items.begin(), items.end(), item);
		if (it == items.end())
			return false;

		items.erase(it);
		return true;
	}

	ActionDefId CreateActionDef(std::string name, ActionTargetType targetType, std::function<void(const ActionContext&)> apply) {
		const auto defIdx = _defs->actions.size();

		ActionDef def;
		def.name = name;
		def.targetType = targetType;
		def.apply = apply;
		_defs->actions.push_back(def);

		return ActionDefId{ defIdx };
	}

	void AddPersonAction(ActionDefId action) {
		_defs->personActions.push_back(action);
	}

	ItemDefId CreateItemDef(std::string name, std::vector<ActionDefId> possibleActions) {
		const auto defIdx = _defs->items.size();

		ItemDef def;
		def.name = name;
		def.possibleActions = possibleActions;
		_defs->items.push_back(def);

		return ItemDefId{ defIdx };
	}

	entt::entity CreateItem(ItemDefId defId) {
		auto entity = _state.create();

		_state.emplace<C::Item>(entity, defId);

		return entity;
	}

	entt::entity CreatePlayer() {
		auto player = CreatePerson("Player");
		_state.emplace<C::Player>(player);
		return player;
	}

	entt::entity CreatePerson(std::string name) {
		const auto defIdx = _defs->persons.size();
		PersonDef def;
		def.name = name;
		_defs->persons.push_back(def);

		auto p = _state.create();
		_state.emplace<C::Person>(p, PersonDefId{ defIdx });
		_state.emplace<C::Inventory>(p);

		return p;
	}

	void KillPerson(entt::entity person) {
		_state.emplace<C::Dead>(person);
	}

	PersonDef& GetPersonDef(entt::entity entity) {
		auto& person = _state.get<C::Person>(entity);
		return _defs->persons[person.defId.value];
	}

	ItemDef& GetItemDef(entt::entity entity) {
		auto& item = _state.get<C::Item>(entity);
		return _defs->items[item.defId.value];
	}

	ActionDef& GetActionDef(ActionDefId actionId) {
		return _defs->actions[actionId.value];
	}

	template<typename VisitorT>
	void VisitPossibleActionsForPerson(entt::entity person, VisitorT& visitor) {
		for (const auto actionDefId : _defs->personActions) {
			const auto& actionDef = _defs->actions[actionDefId.value];

			switch (actionDef.targetType) {
			case ActionTargetType::None:
				visitor(actionDefId); // <--
				break;

			case ActionTargetType::Person:
				for (const auto targetPerson : _state.view<C::Person>()) {
					TargetPerson target;
					target.person = targetPerson;
					visitor(actionDefId, target); // <--
				}
				break;

			case ActionTargetType::ItemOnPerson:
				for (const auto targetEnt : _state.view<C::Person>()) {
					if (targetEnt == person)
						continue;

					auto& targetInventory = _state.get<C::Inventory>(targetEnt);

					for (const auto targetItem : targetInventory.items) {
						TargetItemOnPerson target;
						target.person = targetEnt;
						target.item = targetItem;
						visitor(actionDefId, target); // <--
					}
				}
				break;

			default:
				assert(!"fixme");
			}
		}

		auto& inventory = _state.get<C::Inventory>(person);
		for (const auto itemEntity : inventory.items) {
			const auto& item = _state.get<C::Item>(itemEntity);
			const auto& itemDef = _defs->items[item.defId.value];

			for (const auto actionDefId : itemDef.possibleActions) {
				const auto& actionDef = _defs->actions[actionDefId.value];

				switch (actionDef.targetType) {
				case ActionTargetType::Person:
					for (const auto targetEntity : _state.view<C::Person>()) {
						if (targetEntity == person)
							continue;

						TargetPersonWithItem target;
						target.person = targetEntity;
						target.item = itemEntity;
						visitor(actionDefId, target); // <--
					}
					break;

				default:
					assert(!"fixme");
				}
			}
		}
	}

	void ExecuteActionForPerson(entt::entity actor, size_t actionIdx) {
		struct Visitor {
			size_t m_idx = 0;
			size_t m_executeIdx;
			Game& m_g;
			entt::entity m_actor;

			Visitor(Game& g, entt::entity actor, size_t executeIdx)
				: m_g(g), m_actor(actor), m_executeIdx(executeIdx) {}

			void operator()(ActionDefId actionId) {
				if (m_idx == m_executeIdx) {
					auto& actionDef = m_g.GetActionDef(actionId);
					auto ctx = ActionContext{ m_g, m_actor };
					actionDef.apply(ctx);
				}

				++m_idx;
			}

			void operator()(ActionDefId actionId, const TargetPerson& target) {
				if (m_idx == m_executeIdx) {
					auto& actionDef = m_g.GetActionDef(actionId);
					auto ctx = ActionContext{ m_g, m_actor };
					ctx.targetEntity = target.person;
					actionDef.apply(ctx);
				}

				++m_idx;
			}

			void operator()(ActionDefId actionId, const TargetPersonWithItem& target) {
				if (m_idx == m_executeIdx) {
					auto& actionDef = m_g.GetActionDef(actionId);
					auto ctx = ActionContext{ m_g, m_actor };
					ctx.targetEntity = target.person;
					ctx.targetEntity2 = target.item;
					actionDef.apply(ctx);
				}

				++m_idx;
			}

			void operator()(ActionDefId actionId, const TargetItemOnPerson& target) {
				if (m_idx == m_executeIdx) {
					auto& actionDef = m_g.GetActionDef(actionId);
					auto ctx = ActionContext{ m_g, m_actor };
					ctx.targetEntity = target.item;
					ctx.targetEntity2 = target.person;
					actionDef.apply(ctx);
				}

				++m_idx;
			}
		} visitor(*this, actor, actionIdx);

		VisitPossibleActionsForPerson(actor, visitor);
	}

protected:
	entt::registry _state;
	std::shared_ptr<Defs> _defs;
};
