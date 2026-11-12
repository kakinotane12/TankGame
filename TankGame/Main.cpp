# include <Siv3D.hpp> // Siv3D v0.6.16
# include "Tank.h"     // 自分で作ったヘッダーは "" で囲む（<> は外部ライブラリ）

//
// Stage 3: 弾を撃つ
//
// 操作:
//   W / S : 前進 / 後退
//   A / D : 車体を左回転 / 右回転
//   マウス : 砲塔の向き
//   左クリック : 発射（押しっぱなしで連射。ただし発射間隔の制限あり）
//
// 赤い戦車 dummy はプレイヤーを追いかけ、dummy2 は動かずにマウスを狙う。
// （まだ当たり判定はないので、弾は戦車をすり抜ける）
//

// ------------------------------------------------------------
// キーボードとマウスの状態から、プレイヤー戦車への操作指示を作る
// ------------------------------------------------------------
// 「入力を読む処理」を Tank の外に置くことで，クラス内には処理だけ記載すればよくなる．
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

	// pressed() は「押されている間ずっと true」。
	// 押しっぱなしで撃ち続けられるが、実際の発射間隔は Tank のクールダウンが決める。
	input.fire = MouseL.pressed();

	return input;
}
TankInput MakeChaseInput(const Tank& self, const Vec2& target) {
	TankInput input;
	const Vec2 toTarget = target - self.getPos();
	const double targetAngle = Math::Atan2(toTarget.y, toTarget.x);
	const double diff = Math::NormalizeAngle(targetAngle - self.getBodyAngle(), 0.0);
	const double distance = toTarget.length();  // ベクトルの長さ = 2点間の距離

	//砲塔とターゲットの角度の差分
	const double t_diff = Math::NormalizeAngle(targetAngle - self.getTurretAngle(), 0.0);

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

	if (std::abs(t_diff) < 5_deg) {
		input.fire = true;
	}

	input.aimTarget = target;

	return input;
}

void Main()
{
	Window::SetTitle(U"TankGame - Stage 3");
	Scene::SetBackground(ColorF{ 0.85, 0.80, 0.65 });

	// ------------------------------------------------------------
	// 戦車を作る
	// ------------------------------------------------------------
	// クラスにしたので、同じ設計図から何台でも作れる。
	Tank player{ Scene::Center(), ColorF{ 0.30, 0.45, 0.25 } };
	Tank dummy{ Vec2{ 200, 150 }, ColorF{ 0.60, 0.30, 0.25 } };
	Tank dummy2{ Vec2{ 500, 400 }, ColorF{ 0.70, 0.30, 0.25 } };

	// ------------------------------------------------------------
	// 画面上のすべての弾
	// ------------------------------------------------------------
	// Array は Siv3D の可変長配列（中身は std::vector とほぼ同じ）。
	// 弾は撃つたびに増え、寿命が来たら減るので、数が決まっていない。そういうものは Array で持つ。
	// どの戦車が撃った弾も、ここに集める。
	Array<Bullet> bullets;

	while (System::Update())
	{
		const double deltaTime = Scene::DeltaTime();

		// ============================================================
		// 1. 更新
		// ============================================================
		// --- 戦車（撃った弾は bullets に追加される） ---
		player.update(MakePlayerInput(), deltaTime, bullets);

		// 的の戦車: プレイヤーを狙う
		dummy.update(MakeChaseInput(dummy, player.getPos()), deltaTime, bullets);

		TankInput dummy2Input;
		dummy2Input.aimTarget = Cursor::PosF();
		dummy2.update(dummy2Input, deltaTime, bullets);

		// --- 弾 ---
		// 範囲 for 文: bullets の要素を先頭から1つずつ bullet に入れて繰り返す。
		// bulletsの中の弾丸について，ひとつづつ経過時間を更新
		// auto& と「参照」で受け取る
		for (auto& bullet : bullets)
		{
			bullet.update(deltaTime);
		}

		// 寿命が切れた弾を配列から取り除く。
		// remove_if(条件) は「配列の中で条件が true になる要素」をすべて削除する。
		// [](const Bullet& b) { ... } はラムダ式（その場で書く小さな関数）。
		// 「弾 b を受け取って、生きていなければ true（= 削除する）を返す」という意味。
		bullets.remove_if([](const Bullet& b) { return (not b.isAlive()); });

		// ============================================================
		// 2. 描画
		// ============================================================
		dummy.draw();
		dummy2.draw();
		player.draw(); // 後に描いたほうが上に重なる

		// 弾は戦車より上に描く。描画では弾を変えないので const auto& で受け取る。
		for (const auto& bullet : bullets)
		{
			bullet.draw();
		}

		// --- デバッグ表示 ---
		// player の中身（m_pos など）は private なので直接は読めない。ゲッター経由で読む。
		ClearPrint();
		Print << U"pos    : " << player.getPos();
		Print << U"body   : {:.1f} deg"_fmt(Math::ToDegrees(player.getBodyAngle()));
		Print << U"turret : {:.1f} deg"_fmt(Math::ToDegrees(player.getTurretAngle()));
		Print << U"bullets: " << bullets.size(); // 弾が増え続けず、ちゃんと消えているかの確認用
	}
}
