# include "Result.h"

Result::Result(const InitData& init)
	: IScene{ init }
{
}

void Result::update()
{
	if (MouseL.down() || KeyEnter.down())
	{
		changeScene(U"Title", 0.6s);
	}
}

void Result::draw() const
{
	// draw() は const なので、getData() も const 版（const GameData&）が呼ばれ、読むことしかできない
	const GameData& data = getData();

	if (data.allCleared)
	{
		Scene::SetBackground(ColorF{ 0.18, 0.26, 0.20 });
		FontAsset(U"Title")(U"ALL CLEAR!").drawAt(TextStyle::Outline(0.2, ColorF{ 0.1 }), 72, Vec2{ 400, 220 }, ColorF{ 0.95, 0.85, 0.40 });
		FontAsset(U"Medium")(U"全 {} ステージを突破した！"_fmt(StageCount)).drawAt(Vec2{ 400, 320 }, ColorF{ 0.95 });
	}
	else
	{
		Scene::SetBackground(ColorF{ 0.25, 0.15, 0.15 });
		FontAsset(U"Title")(U"GAME OVER").drawAt(TextStyle::Outline(0.2, ColorF{ 0.1 }), 72, Vec2{ 400, 220 }, ColorF{ 0.90, 0.40, 0.35 });
		FontAsset(U"Medium")(U"到達ステージ: {} / {}"_fmt(data.stage, StageCount)).drawAt(Vec2{ 400, 320 }, ColorF{ 0.95 });
	}

	if (Periodic::Square0_1(1.2s))
	{
		FontAsset(U"Medium")(U"クリックでタイトルへ").drawAt(Vec2{ 400, 440 }, ColorF{ 0.85 });
	}
}
