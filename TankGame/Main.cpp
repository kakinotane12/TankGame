# include <Siv3D.hpp> // Siv3D v0.6.16

//
// Stage 1: 戦車を動かす
//
// 操作:
//   W / S : 前進 / 後退（車体の向きに沿って進む）
//   A / D : 車体を左回転 / 右回転
//   マウス : 砲塔がマウスカーソルの方を向く
//
//

// ------------------------------------------------------------
// 定数
// ------------------------------------------------------------
// constexpr = コンパイル時に値が決まる定数。

// 前進・後退の速さ [ピクセル/秒]
constexpr double MoveSpeed = 150.0;
constexpr double BackSpeed = 75.0;

// 車体の回転の速さ [ラジアン/秒]
// 90_deg は Siv3D のリテラルで「90度をラジアンに変換した値」(= π/2) になる。
constexpr double RotateSpeed = 90_deg;

// 砲塔の回転の速さ [ラジアン/秒]
constexpr double TurretRotateSpeed = 90_deg;

// 車体の大きさ（前後の長さ × 左右の幅）[ピクセル]
constexpr SizeF BodySize{ 48, 36 };

// 砲塔（丸い部分）の半径と、砲身の長さ・太さ [ピクセル]
constexpr double TurretRadius = 12.0;
constexpr double BarrelLength = 30.0;
constexpr double BarrelWidth = 8.0;

void Main()
{
	Window::SetTitle(U"TankGame - Stage 1");
	Scene::SetBackground(ColorF{ 0.85, 0.80, 0.65 });

	// ------------------------------------------------------------
	// 戦車の状態（ゲームループの外で宣言 → 毎フレーム値が引き継がれる）
	// ------------------------------------------------------------
	// Vec2 は (x, y) を持つ 2次元ベクトル。中身は double 2つ。
	// Scene::Center() は画面中央の座標（Point 型）なので、Vec2 に変換して受け取る。
	Vec2 tankPos = Scene::Center();

	// 車体の向き [ラジアン]
	// 0 = 右向き。角度が増えると時計回りに回る。
	double bodyAngle = 0.0;

	// 砲塔の向き [ラジアン]（車体とは独立して回る）
	double turretAngle = 0.0;

	while (System::Update())
	{
		// ============================================================
		// 1. 更新（入力を読んで、状態を変える）
		// ============================================================

		// 前フレームからの経過時間 [秒]。60fps なら約 0.0167。
		// 「速さ × 経過時間」で移動量を求めると、fps が変わってもゲームの速さが一定になる。
		const double deltaTime = Scene::DeltaTime();

		// --- 車体の回転 ---
		if (KeyA.pressed())
		{
			bodyAngle -= RotateSpeed * deltaTime; // 反時計回り
		}
		if (KeyD.pressed())
		{
			bodyAngle += RotateSpeed * deltaTime; // 時計回り
		}

		// --- 前進・後退 ---
		// 車体が向いている方向の単位ベクトル（長さ 1 のベクトル）を三角関数で作る。
		//   角度 θ の方向 = (cos θ, sin θ)
		const Vec2 forward{ Math::Cos(bodyAngle), Math::Sin(bodyAngle) };

		if (KeyW.pressed())
		{
			tankPos += forward * MoveSpeed * deltaTime;
		}
		if (KeyS.pressed())
		{
			tankPos -= forward * BackSpeed * deltaTime;
		}

		// --- 画面外に出ないようにする ---
		// Clamp(値, 最小, 最大) は値を範囲内に収める関数。
		// 車体の半分の大きさだけ内側に制限する。
		const double margin = BodySize.x / 2;
		tankPos.x = Clamp(tankPos.x, margin, Scene::Width() - margin);
		tankPos.y = Clamp(tankPos.y, margin, Scene::Height() - margin);

		// --- 砲塔をマウスの方へ向ける ---
		// 戦車からマウスへのベクトルを求め、その角度を atan2 で計算する。
		//   atan2(y, x) は「ベクトル (x, y) が x 軸から何ラジアン回っているか」を返す。
		const Vec2 toCursor = Cursor::PosF() - tankPos;
		const double targetAngle = Math::Atan2(toCursor.y, toCursor.x);

		// 現在の砲塔の向きとマウスの向きの差分を求める
		const double diff = Math::NormalizeAngle(targetAngle - turretAngle, 0.0);
		// 目標との差を比較，超えていたら最大角度分しか動かさない
		const double maxStep = TurretRotateSpeed * deltaTime;
		turretAngle += Clamp(diff, -maxStep, maxStep);

		// ============================================================
		// 2. 描画（状態をもとに絵を描くだけ。状態は変えない）
		// ============================================================

		// --- 車体 ---
		{
			// Transformer2D: このブロック内の描画に「座標変換」をかける。
			//   Rotate(bodyAngle)       … 原点まわりに回転して
			//   .translated(tankPos)    … 戦車の位置へ平行移動する
			// これにより「戦車の中心を (0, 0)、前方を +x とした座標（ローカル座標）」で描ける。
			// 回転後の頂点座標を自分で計算する必要がなくなる。
			const Transformer2D transform{ Mat3x2::Rotate(bodyAngle).translated(tankPos) };

			// 履帯（キャタピラ）: 車体の左右に細長い長方形
			// Arg::center で「中心座標とサイズ」から長方形を作れる。
			RectF{ Arg::center(0, -BodySize.y / 2), BodySize.x + 4, 8 }.draw(ColorF{ 0.25 });
			RectF{ Arg::center(0, BodySize.y / 2), BodySize.x + 4, 8 }.draw(ColorF{ 0.25 });

			// 車体本体
			RectF{ Arg::center(0, 0), BodySize }.draw(ColorF{ 0.30, 0.45, 0.25 });

			// 前方の目印（前がどちらか分かるように）
			RectF{ Arg::center(BodySize.x / 2 - 4, 0), 6, 20 }.draw(ColorF{ 0.20, 0.32, 0.18 });
		} // ← ここで transform が破棄され、座標変換が自動で解除される（RAII）

		// --- 砲塔 ---
		{
			// 砲塔は車体とは別の角度で回るので、別の Transformer2D を使う。
			const Transformer2D transform{ Mat3x2::Rotate(turretAngle).translated(tankPos) };

			// 砲身: 中心から +x 方向へ伸びる長方形
			RectF{ 0, -BarrelWidth / 2, BarrelLength, BarrelWidth }.draw(ColorF{ 0.20, 0.30, 0.18 });

			// 砲塔の丸い部分
			Circle{ 0, 0, TurretRadius }.draw(ColorF{ 0.38, 0.55, 0.32 });
		}

		// --- デバッグ表示 ---
		// ClearPrint() で前フレームの表示を消してから Print する。
		// Math::ToDegrees でラジアンを度に直すと人間に読みやすい。
		ClearPrint();
		Print << U"pos    : " << tankPos;
		Print << U"body   : {:.1f} deg"_fmt(Math::ToDegrees(bodyAngle));
		Print << U"turret : {:.1f} deg"_fmt(Math::ToDegrees(turretAngle));
	}
}
