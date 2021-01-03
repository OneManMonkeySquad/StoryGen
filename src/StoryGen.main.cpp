#include "Game.h"
#include "Planner.h"
#include <windows.h>


void InitTestGame(Game& g, Planner& p) {
	///////////////////////////////////////// Setup /////////////////////////////////////////
	const auto sleep = g.CreateActionDef("sleep", ActionTargetType::None, [](Game&, const ActionRef&) {});
	const auto steal = g.CreateActionDef("steal", ActionTargetType::ItemOnPerson, [](Game& g, const ActionRef& ref) {
		g.RemoveItem(ref.target, ref.targetItem);
		g.AddItem(ref.actor, ref.targetItem);
		});
	g.AddPersonAction(sleep);
	g.AddPersonAction(steal);

	const auto stab = g.CreateActionDef("stab", ActionTargetType::Person, [](Game& g, const ActionRef& ref) {
		g.KillPerson(ref.target);
		});
	const auto knifeDef = g.CreateItemDef("knife", { stab });

	const auto bribe = g.CreateActionDef("bribe", ActionTargetType::Person, [](Game& g, const ActionRef& ref) {
		g.RemoveItem(ref.actor, ref.item);
		g.AddItem(ref.target, ref.item);
		});
	const auto coinDef = g.CreateItemDef("coin", { bribe });

	auto player = g.CreatePlayer();

	auto queen = g.CreatePerson("Queen");
	g.AddItem(queen, g.CreateItem(knifeDef));
	g.AddItem(queen, g.CreateItem(coinDef));

	auto king = g.CreatePerson("King");
	g.AddItem(king, g.CreateItem(coinDef));

	auto soldier = g.CreatePerson("Soldier");

	///////////////////////////////////////// Story /////////////////////////////////////////
	p.AddGoal({ king, bribe, soldier });
	p.AddGoal({ soldier, stab, queen });
}

void PrintPlayerActions(const Game& g, const std::vector<ActionRef>& actions) {
	printf("\x1b[30;47m");
	for (size_t i = 0; i < actions.size(); ++i) {
		auto& action = actions[i];
		auto desc = g.GetActionDescription(action);
		printf("%d. %s\n", static_cast<int>(i + 1), desc.c_str());
	}
	printf("\x1b[m");
}

void RunGame(Game& g, Planner& p) {
	g.DescribeState();
	printf("\n\n");

	while (true) {
		const auto player = g.GetPlayer();
		const auto possiblePlayerActions = g.CalculatePossibleActionsForPerson(player);

		PrintPlayerActions(g, possiblePlayerActions);

		auto c = _getch();
		if (c == 27)
			break;

		printf("\x1B[0G");

		const auto playerActionIdx = c - 49;
		if (playerActionIdx < 0 || playerActionIdx >= possiblePlayerActions.size())
			continue;

		const auto& playerAction = possiblePlayerActions[playerActionIdx];
		printf("%s.", g.GetActionDescription(playerAction).c_str());

		g.ExecuteActionForPerson(playerAction);

		auto plan = p.CalculatePlan();
		if (plan) {
#if 1
			printf("Plan: {");
			for (auto step : *plan) {
				printf(" %s", g.GetActionDescription(step).c_str());
			}
			printf("}\n");
#endif

			auto firstStep = (*plan)[0];
			g.ExecuteActionForPerson(firstStep);

			printf(" %s.\n", g.GetActionDescription(firstStep).c_str());

			if (plan->size() == 1) {
				p.NextGoal();
			}
		}

		printf("\n");
	}
}

bool InitConsole() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		return false;

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
		return false;

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
		return false;

	return true;
}

int main(int argc, char** argv) {
	if (!InitConsole())
		return GetLastError();

	Game g;
	Planner p{ g };
	InitTestGame(g, p);
	RunGame(g, p);

	return 0;
}