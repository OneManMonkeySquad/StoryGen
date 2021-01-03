#pragma once

#include <optional>
#include <vector>

struct Goal {
	entt::entity person;
	ActionDefId actionId;
	entt::entity target;
};

class Planner {
public:
	Planner(Game& g) : _g(g) {}

	void AddGoal(Goal goal) {
		_goal.push_back(goal);
	}

	std::optional<std::vector<ActionRef>> CalculatePlan() {
		const auto& currentGoal = _goal[0];

		std::vector<Partial> todo;
		todo.push_back(Partial{ _g.Clone(), {} });

		std::vector< std::vector<ActionRef > > results;
		size_t resultMinStep = 4;

		while (!todo.empty()) {
			auto partial = std::move(todo.back());
			todo.pop_back();

			const auto MAX_STEPS = 4;
			if (partial.steps.size() > resultMinStep)
				continue;

			auto possibleActions = partial.state.CalculatePossibleActionsForPerson(currentGoal.person);

			for (size_t i = 0; i < possibleActions.size(); ++i) {
				Partial newPartial{ partial.state.Clone(), partial.steps };
				newPartial.steps.push_back(possibleActions[i]);

				if (possibleActions[i].actionId.value == currentGoal.actionId.value
					&& possibleActions[i].target == currentGoal.target) {
					resultMinStep = std::min(resultMinStep, newPartial.steps.size());
					results.push_back(newPartial.steps);
					continue;
				}

				newPartial.state.ExecuteActionForPerson(possibleActions[i]);

				todo.push_back(std::move(newPartial));
			}
		}

		size_t shortestSize = 999;
		int shortestIdx = -1;
		for (size_t i = 0; i < results.size(); ++i) {
			if (results[i].size() < shortestSize) {
				shortestSize = results[i].size();
				shortestIdx = (int)i;
			}
		}

		if (shortestIdx != -1)
			return results[shortestIdx];

		return {};
	}

	void NextGoal() {
		_goal.erase(_goal.begin());
	}

private:
	Game& _g;
	std::vector<Goal> _goal;

	struct Partial {
		Game state;
		std::vector<ActionRef> steps;
	};
};
