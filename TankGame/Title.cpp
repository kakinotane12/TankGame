# include "Title.h"
# include "Tank.h"

Title::Title(const InitData& init)
	: IScene{ init } // 基底クラスのコンストラクタに init を渡す
{
}

void Title::update()
{
	// クリックまたは Enter でゲーム開始
	if (MouseL.down() || KeyEnter.down())
	{
		// 新しいゲームを始めるので、共有データを最初の状態に戻す
		// getData() は GameData& を返すので、書き換えは全シーンに反映される
		GameData& data = getData();
		data.stage = 1;
		data.lives = InitialLives;
		data.allCleared = false;

		// 0.6 秒かけてフェードしながら "Game" シーンへ切り替える
		changeScene(U"Game", 0.6s);
	}
}

void Title::draw() const
{
	Scene::SetBackground(ColorF{ 0.20, 0.24, 0.18 });

	// タイトル（MSDF フォントは drawAt の第1引数で大きさを自由に変えられる）
	FontAsset(U"Title")(U"TANK GAME").drawAt(TextStyle::Outline(0.2, ColorF{ 0.1 }), 80, Vec2{ 400, 170 }, ColorF{ 0.85, 0.80, 0.65 });

	// 飾りの戦車（Stage 2 で作った Tank クラスを、描画だけに使う）
	{
		const Tank tank{ Vec2{ 400, 300 }, ColorF{ 0.30, 0.45, 0.25 }, Team::Player };
		const Transformer2D scale{ Mat3x2::Scale(2.0, Vec2{ 400, 300 }) }; // (400, 300) を中心に 2 倍に拡大
		tank.draw();
	}

	// 点滅する案内（Periodic::Square0_1(周期) は 0 と 1 を交互に返す）
	if (Periodic::Square0_1(1.2s))
	{
		FontAsset(U"Medium")(U"クリックでスタート").drawAt(Vec2{ 400, 420 }, ColorF{ 0.95 });
	}

	// 操作説明
	FontAsset(U"Small")(U"W / S : 前進・後退    A / D : 旋回    マウス : 照準    左クリック : 発射").drawAt(Vec2{ 400, 520 }, ColorF{ 0.75 });
	FontAsset(U"Small")(U"F2 : AI のデバッグ表示    Esc : 終了").drawAt(Vec2{ 400, 545 }, ColorF{ 0.6 });
}
