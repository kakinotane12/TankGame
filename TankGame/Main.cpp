# include <Siv3D.hpp> // Siv3D v0.6.16
# include "Tank.h"     // 自分で作ったヘッダーは "" で囲む（<> は外部ライブラリ）

//
// Stage 2: Tank クラスとファイル分割
//
// 操作:
//   W / S : 前進 / 後退
//   A / D : 車体を左回転 / 右回転
//   マウス : 砲塔の向き
//
// 画面左上の赤い戦車は動かない「的」で、プレイヤーを狙い続ける。
//

// ------------------------------------------------------------
// キーボードとマウスの状態から、プレイヤー戦車への操作指示を作る
// ------------------------------------------------------------
// 「入力を読む処理」を Tank の外に置いておくことで、
// Tank は「指示どおりに動く」ことだけに集中できる。
TankInput MakePlayerInput()
{
	TankInput input;

	// W と S を同時に押したら打ち消し合って 0 になる
	if (KeyW.pressed())
	{
		input.move += 1.0;
	}
	if (KeyS.pressed())
	{
		input.move -= 1.0;
	}

	if (KeyD.pressed())
	{
		input.rotate += 1.0;
	}
	if (KeyA.pressed())
	{
		input.rotate -= 1.0;
	}

	input.aimTarget = Cursor::PosF();

	return input;
}
TankInput MakeChaseInput(const Tank& self, const Vec2& target) {
	TankInput input;
	const Vec2 toTarget = target - self.getPos();
	const double targetAngle = Math::Atan2(toTarget.y, toTarget.x);
	const double diff = Math::NormalizeAngle(targetAngle - self.getBodyAngle(), 0.0);
	const double distance = toTarget.length();  // ベクトルの長さ = 2点間の距離

	if (diff > 0) {
		input.rotate = 1.0;
	}
	else if (diff < 0) {
		input.rotate = -1.0;
	}
	else {
		input.rotate = 0;
	}
	if (std::abs(diff) < 10_deg && distance > 120) {
		input.move = 1.0;
	}
	else {
		input.move = 0.0;
	}

	input.aimTarget = target;

	return input;
}

void Main()
{
	Window::SetTitle(U"TankGame - Stage 2");
	Scene::SetBackground(ColorF{ 0.85, 0.80, 0.65 });

	// ------------------------------------------------------------
	// 戦車を作る
	// ------------------------------------------------------------
	// クラスにしたので、同じ設計図から何台でも作れる。
	// 変数の作り方は int x{ 5 }; と同じで、{ } の中身がコンストラクタの引数になる。
	Tank player{ Scene::Center(), ColorF{ 0.30, 0.45, 0.25 } };
	Tank dummy{ Vec2{ 200, 150 }, ColorF{ 0.60, 0.30, 0.25 } };
	Tank dummy2{ Vec2{ 500, 400 }, ColorF{ 0.70, 0.30, 0.25 } };

	while (System::Update())
	{
		const double deltaTime = Scene::DeltaTime();

		// ============================================================
		// 1. 更新
		// ============================================================
		player.update(MakePlayerInput(), deltaTime);

		// 的の戦車: プレイヤーを狙う
		dummy.update(MakeChaseInput(dummy, player.getPos()), deltaTime);

		TankInput dummy2Input;
		dummy2Input.aimTarget = Cursor::PosF();
		dummy2.update(dummy2Input, deltaTime);


		// ============================================================
		// 2. 描画
		// ============================================================
		dummy.draw();
		dummy2.draw();
		player.draw(); // 後に描いたほうが上に重なる

		// --- デバッグ表示 ---
		// player の中身（m_pos など）は private なので直接は読めない。ゲッター経由で読む。
		ClearPrint();
		Print << U"pos    : " << player.getPos();
		Print << U"body   : {:.1f} deg"_fmt(Math::ToDegrees(player.getBodyAngle()));
		Print << U"turret : {:.1f} deg"_fmt(Math::ToDegrees(player.getTurretAngle()));
	}
}
