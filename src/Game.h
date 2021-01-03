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

	PersonDef& GetPersonDef(entt::entity entity) const {
		auto& person = _state.get<C::Person>(entity);
		return _defs->persons[person.defId.value];
	}

	ItemDef& GetItemDef(entt::entity entity) const {
		auto& item = _state.get<C::Item>(entity);
		return _defs->items[item.defId.value];
	}

	ActionDef& GetActionDef(ActionDefId actionId) const {
		return _defs->actions[actionId.value];
	}

	void DescribeState() {
		printf("There's ");
		auto persons = _state.view<const C::Person>();
		for (auto it = persons.begin(); it != persons.end(); ++it) {
			const auto& person = _state.get<C::Person>(*it);
			const auto& personDef = _defs->persons[person.defId.value];
			const auto isDead = _state.has<C::Dead>(*it);

			printf("%s", personDef.name.c_str());
			if (isDead) {
				printf(" (dead)");
			}
			if (it + 2 < persons.end()) {
				printf(", ");
			}
			if (it + 2 == persons.end()) {
				printf(" and ");
			}
		}

		printf(". ");
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

	ActionDefId CreateActionDef(std::string name, ActionTargetType targetType, std::function<void(Game&, const ActionRef&)> apply) {
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
		auto player = CreatePerson("you");
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

	std::string GetActionDescription(const ActionRef& ref) const {
		auto action = GetActionDef(ref.actionId);
		auto& actor = GetPersonDef(ref.actor);
		auto* target = ref.target != entt::null ? &GetPersonDef(ref.target) : nullptr;

		char buff[512];
		

		switch (action.targetType) {
		case ActionTargetType::None:
			snprintf(buff, std::size(buff), "%s %s",
				actor.name.c_str(),
				action.name.c_str());
			break;

		case ActionTargetType::ItemOnPerson:
			snprintf(buff, std::size(buff), "%s %s %s from %s",
				actor.name.c_str(),
				action.name.c_str(),
				GetItemDef(ref.targetItem).name.c_str(),
				target ? target->name.c_str() : "");
			break;

		default:
			snprintf(buff, std::size(buff), "%s %s %s",
				actor.name.c_str(),
				action.name.c_str(),
				target ? target->name.c_str() : "");
			break;
		}

		// Player action : <you, Sleep, >
		// Narrator action : <King, Steal, Queen>

		return buff;
	}

	entt::entity GetPlayer() {
		auto player = _state.view<C::Player>().front();
		return player;
	}

	void ExecuteActionForPerson(const ActionRef& ref) {
		const auto& actionDef = _defs->actions[ref.actionId.value];
		actionDef.apply(*this, ref);
	}

	std::vector<ActionRef> CalculatePossibleActionsForPerson(entt::entity person) {
		std::vector<ActionRef> result;
		VisitPossibleActionsForPerson(person, [&](const ActionRef& ref) { result.push_back(ref); });
		return result;
	}

	template<typename VisitorT>
	void VisitPossibleActionsForPerson(entt::entity person, VisitorT&& visitor) {
		for (const auto actionDefId : _defs->personActions) {
			const auto& actionDef = _defs->actions[actionDefId.value];

			ActionRef ref;
			ref.actionId = actionDefId;
			ref.actor = person;

			switch (actionDef.targetType) {
			case ActionTargetType::None:
				visitor(ref); // <--
				break;

			case ActionTargetType::Person:
				for (const auto targetPerson : _state.view<C::Person>(entt::exclude<C::Dead>)) {
					ref.target = targetPerson;
					visitor(ref); // <--
				}
				break;

			case ActionTargetType::ItemOnPerson:
				for (const auto targetEnt : _state.view<C::Person>()) {
					if (targetEnt == person)
						continue;

					auto& targetInventory = _state.get<C::Inventory>(targetEnt);

					for (const auto targetItem : targetInventory.items) {
						ref.target = targetEnt;
						ref.targetItem = targetItem;
						visitor(ref); // <--
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

				ActionRef ref;
				ref.actionId = actionDefId;
				ref.actor = person;

				switch (actionDef.targetType) {
				case ActionTargetType::Person:
					for (const auto targetEntity : _state.view<C::Person>(entt::exclude<C::Dead>)) {
						if (targetEntity == person)
							continue;

						ref.target = targetEntity;
						ref.item = itemEntity;
						visitor(ref); // <--
					}
					break;

				default:
					assert(!"fixme");
				}
			}
		}
	}

protected:
	entt::registry _state;
	std::shared_ptr<Defs> _defs;
};
