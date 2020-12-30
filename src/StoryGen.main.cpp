#include "Game.h"
#include <windows.h>
#include <optional>


// <person, action, target>
struct Goal {
	entt::entity person;
	ActionDefId actionDef;
	entt::entity target;
};

class Planner {
public:
	Planner(Game& g) : _g(g) {}

	void AddGoal(Goal goal) {
		_goal.push_back(goal);
	}

	std::optional<std::vector<Goal>> Run() {
		const auto& currentGoal = _goal[0];

		std::vector<Partial> todo;
		todo.push_back(Partial{ _g.Clone(), {} });

		while (!todo.empty()) {
			auto partial = std::move(todo.back());
			todo.pop_back();

			const auto MAX_STEPS = 4;
			if (partial.steps.size() > MAX_STEPS)
				continue;

			struct Visitor {
				entt::entity _person;
				std::vector<Goal> poo;
				Visitor(entt::entity person) : _person(person) {}
				void operator()(ActionDefId actionId, const TargetPersonWithItem& target) {
					poo.push_back(Goal{ _person, actionId, target.person });
				}
				void operator()(ActionDefId actionId, const TargetItemOnPerson& target) {
					poo.push_back(Goal{ _person, actionId, target.person });
				}
				void operator()(ActionDefId actionId, ...) {
					poo.push_back(Goal{ _person, actionId, entt::null });
				}
			} visitor(currentGoal.person);

			partial.state.VisitPossibleActionsForPerson(currentGoal.person, visitor);

			for (size_t i = 0; i < visitor.poo.size(); ++i) {
				Partial newPartial{ partial.state.Clone(), partial.steps };
				newPartial.steps.push_back(visitor.poo[i]);

				if (visitor.poo[i].actionDef.value == currentGoal.actionDef.value
					&& visitor.poo[i].target == currentGoal.target)
					return newPartial.steps;

				newPartial.state.ExecuteActionForPerson(currentGoal.person, i);

				todo.push_back(std::move(newPartial));
			}
		}

		return {};
	}

private:
	Game& _g;
	std::vector<Goal> _goal;

	struct Partial {
		Game state;
		std::vector<Goal> steps;
	};
};

void InitTestGame(Game& g, Planner& p) {
	///////////////////////////////////////// Setup /////////////////////////////////////////
	const auto idle = g.CreateActionDef("Sleep", ActionTargetType::None, [](const ActionContext& ctx) {});
	const auto steal = g.CreateActionDef("Steal", ActionTargetType::ItemOnPerson, [](const ActionContext& ctx) {
		ctx.g.RemoveItem(ctx.targetEntity2, ctx.targetEntity);
		ctx.g.AddItem(ctx.actor, ctx.targetEntity);
		});
	g.AddPersonAction(idle);
	g.AddPersonAction(steal);

	const auto stab = g.CreateActionDef("Stab", ActionTargetType::Person, [](const ActionContext& ctx) {
		ctx.g.KillPerson(ctx.targetEntity);
		});
	const auto knifeDef = g.CreateItemDef("Knife", { stab });

	auto player = g.CreatePlayer();

	auto chloe = g.CreatePerson("Chloe");
	g.AddItem(chloe, g.CreateItem(knifeDef));

	auto king = g.CreatePerson("King");


	///////////////////////////////////////// Story /////////////////////////////////////////
	// <person, action, target>

	p.AddGoal({ king, stab, chloe });

	auto plan = p.Run();
	if (plan) {
		printf("Plan:\n");
		for (auto step : *plan) {
			auto& actionDef = g.GetActionDef(step.actionDef);
			auto& actor = g.GetPersonDef(step.person);
			auto& target = g.GetPersonDef(step.target);
			printf("  <%s,%s,%s>\n", actor.name.c_str(), actionDef.name.c_str(), target.name.c_str());
		}
	}
}

void RunGame(Game& g) {
	g.DescribeState();
	printf("\n\n");

	while (true) {
		g.PrintPlayerActions();

		auto c = _getch();
		if (c == 27)
			break;

		printf("\x1B[0G");

		g.ExecutePlayerAction(static_cast<size_t>(c - 49));

		printf("\n");
	}
}


int main(int argc, char** argv) {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		return GetLastError();

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
		return GetLastError();

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
		return GetLastError();

	Game g;
	Planner p{ g };
	InitTestGame(g, p);
	RunGame(g);

	return 0;
}