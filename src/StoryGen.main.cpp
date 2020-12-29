#include "Game.h"

class TestGame : public Game {
public:
	TestGame() {
		const auto idle = CreateActionDef("Sleep", ActionTargetType::None, [](const ActionContext& ctx) {});
		const auto steal = CreateActionDef("Steal", ActionTargetType::ItemOnPerson, [](const ActionContext& ctx) {
			printf("Stealing...\n");
			});
		AddPersonAction(idle);
		AddPersonAction(steal);

		const auto stab = CreateActionDef("Stab", ActionTargetType::Person, [](const ActionContext& ctx) {
			const auto& defs = ctx.state.ctx<Defs>();

			const auto& person = ctx.state.get<C::Person>(ctx.targetEntity);
			const auto& personDef = defs.persons[person.defId.value];

			ctx.state.emplace<C::Dead>(ctx.targetEntity);

			printf("%s got stabbed and is dead now.\n", personDef.name.c_str());
			});
		const auto knifeDef = CreateItemDef("Knife", { stab });

		auto player = CreatePerson("Player");
		m_state.emplace<C::Player>(player);
		AddItem(player, CreateItem(knifeDef));


		const auto boomerangDef = CreateItemDef("Boomerang", {});

		auto chloe = CreatePerson("Chloe");
		AddItem(chloe, CreateItem(boomerangDef));

		auto king = CreatePerson("King");
	}
};

int main(int argc, char** argv) {
	TestGame game;
	game.run();

	return 0;
}