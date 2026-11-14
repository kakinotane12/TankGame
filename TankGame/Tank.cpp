# include "Tank.h"

//
// Tank.cpp には Tank.h で宣言した関数の「定義（中身）」を書く。
//

// ------------------------------------------------------------
// 定数（このファイルの中だけで使う）
// ------------------------------------------------------------
// 名前のない namespace（無名名前空間）で囲むと、中身は「このファイルの中だけ」で見える。
// 他の .cpp に同じ名前の定数があってもぶつからない。
// Tank の調整値は Tank.cpp だけが知っていればよいので、ここに閉じ込める。
namespace
{
	// 前進・後退の速さ [ピクセル/秒]
	constexpr double MoveSpeed = 150.0;
	constexpr double BackSpeed = 75.0;

	// 車体の回転の速さ [ラジアン/秒]
	constexpr double BodyRotateSpeed = 90_deg;

	// 砲塔の回転の速さ [ラジアン/秒]
	constexpr double TurretRotateSpeed = 45_deg;

	// 車体の大きさ（前後の長さ × 左右の幅）[ピクセル]
	constexpr SizeF BodySize{ 48, 36 };

	// 砲塔（丸い部分）の半径と、砲身の長さ・太さ [ピクセル]
	constexpr double TurretRadius = 12.0;
	constexpr double BarrelLength = 30.0;
	constexpr double BarrelWidth = 8.0;

	// 弾の速さ [ピクセル/秒]
	constexpr double BulletSpeed = 300.0;

	// 発射間隔（クールダウン）[秒]。撃ってからこの時間は次を撃てない。
	constexpr double FireInterval = 0.5;

	// ---- 射撃のばらつき（Stage 8）[ラジアン] ----
	// 「狙いの中心から最大何度ずれるか」を表す。状態ごとに足し算で決める。
	constexpr double BaseSpread = 1.5_deg;        // 完全に止まっているとき
	constexpr double MoveSpread = 6.0_deg;        // 全速で前後に動いているときに足される分
	constexpr double BodyTurnSpread = 3.0_deg;    // 車体を回しているときに足される分
	constexpr double TurretTurnSpread = 4.0_deg;  // 砲塔を全速で回しているときに足される分
	constexpr double FireKick = 4.0_deg;          // 1発撃つごとに反動で広がる分
	constexpr double MaxSpread = 15.0_deg;        // これ以上は広がらない

	// 目標のばらつきに近づく速さ [1/秒]。大きいほど早く落ち着く。
	constexpr double SpreadSettleRate = 4.0;
}

// ------------------------------------------------------------
// コンストラクタ
// ------------------------------------------------------------
// 「Tank::」は「Tank クラスの」という意味（スコープ解決演算子）。
// : の後ろはメンバ初期化子リスト。メンバ変数を作る時点で値を入れる。
// （{ } の中で m_pos = pos; と代入するより効率がよく、const メンバも初期化できる）
Tank::Tank(const Vec2& pos, const ColorF& color, const Team team)
	: m_pos{ pos }
	, m_color{ color }
	, m_team{ team }
	, m_spread{ BaseSpread }
{
}

// ------------------------------------------------------------
// 更新
// ------------------------------------------------------------
Vec2 Tank::getMuzzlePos() const
{
	return m_pos + Vec2{ Math::Cos(m_turretAngle), Math::Sin(m_turretAngle) } * BarrelLength;
}

void Tank::update(const TankInput& input, const double deltaTime, const Map& map, Array<Bullet>& bullets)
{
	// 「このフレームで撃ったか」は毎フレーム最初に false に戻し、撃ったときだけ true にする
	m_firedThisFrame = false;

	updateBody(input, deltaTime, map);
	const double turretTurnRatio = updateTurret(input.aimTarget, deltaTime);
	updateSpread(input, turretTurnRatio, deltaTime);
	updateFire(input.fire, deltaTime, map, bullets);
}

void Tank::updateBody(const TankInput& input, const double deltaTime, const Map& map)
{
	// --- 車体の回転 ---
	// Stage 1 では if (KeyA) / if (KeyD) で分けていたが、
	// input.rotate が -1 / 0 / +1 なので、掛け算1回で済む。
	m_bodyAngle += input.rotate * BodyRotateSpeed * deltaTime;

	// --- 前進・後退 ---
	const Vec2 forward{ Math::Cos(m_bodyAngle), Math::Sin(m_bodyAngle) };

	// 三項演算子: (条件) ? (真のときの値) : (偽のときの値)
	const double speed = (0.0 <= input.move) ? MoveSpeed : BackSpeed;
	m_pos += forward * input.move * speed * deltaTime;

	// --- 壁にめり込んだら押し戻す ---
	// 「動いてから、めり込んだ分を戻す」方式。壁に斜めにぶつかると、壁に沿って滑るように動く。
	// （Stage 3 までの「画面外に出ない」処理は、マップの外周の壁が代わりに担う）
	m_pos = map.pushOut(m_pos, CollisionRadius);
}

double Tank::updateTurret(const Vec2& aimTarget, const double deltaTime)
{
	// Stage 1 の練習課題 3 で作った「砲塔をゆっくり回す」処理
	const Vec2 toTarget = aimTarget - m_pos;
	const double targetAngle = Math::Atan2(toTarget.y, toTarget.x);

	// 今の向きと目標の向きの差（-π 〜 π に正規化して、近いほうから回る）
	const double diff = Math::NormalizeAngle(targetAngle - m_turretAngle, 0.0);

	// 1フレームで回れる量を超えないようにする
	const double maxStep = TurretRotateSpeed * deltaTime;
	const double step = Clamp(diff, -maxStep, maxStep);
	m_turretAngle += step;

	// 全速で回した = 1.0。deltaTime が 0 のフレーム（0 で割る）に備えて場合分けする。
	return (0.0 < maxStep) ? (Math::Abs(step) / maxStep) : 0.0;
}

void Tank::updateSpread(const TankInput& input, const double turretTurnRatio, const double deltaTime)
{
	// --- 今の状態から「目標のばらつき」を決める ---
	// 動きの大きさ（0〜1）に比例して足していく。move / rotate は -1〜+1 なので Abs で大きさにする。
	const double targetSpread = BaseSpread
		+ MoveSpread * Math::Abs(input.move)
		+ BodyTurnSpread * Math::Abs(input.rotate)
		+ TurretTurnSpread * turretTurnRatio;

	// --- 今のばらつきを、目標へ少しずつ近づける ---
	// 「差に一定の割合を掛けて縮める」をフレームごとにやると、FPS によって速さが変わってしまう。
	// Exp(-速さ × 経過時間) を掛けると、FPS が 60 でも 144 でも同じ時間で同じだけ近づく。
	//   例: SpreadSettleRate = 4, 0.5 秒後 → 差が Exp(-2) ≒ 0.14 倍（86% 近づく）
	m_spread = targetSpread + (m_spread - targetSpread) * Math::Exp(-SpreadSettleRate * deltaTime);
}

void Tank::updateFire(const bool fire, const double deltaTime, const Map& map, Array<Bullet>& bullets)
{
	// クールダウンの残り時間を減らす（0 より小さくはしない）
	m_fireCooldown = Max(m_fireCooldown - deltaTime, 0.0);

	// 撃ちたくない、またはまだ撃てないなら何もしない（早期 return）
	if ((not fire) || (0.0 < m_fireCooldown))
	{
		return;
	}

	// 砲塔の向きの単位ベクトル
	const Vec2 direction{ Math::Cos(m_turretAngle), Math::Sin(m_turretAngle) };

	// 砲身の先端（戦車の中心から、砲塔の向きに砲身の長さだけ進んだ位置）から撃つ
	const Vec2 muzzlePos = m_pos + direction * BarrelLength;

	// 壁に密着して砲身の先が壁の中にあるときは撃たない（弾が壁の中に生まれてしまうため）
	// このときはクールダウンも消費しない
	if (map.isWallAt(muzzlePos))
	{
		return;
	}

	// --- 射撃のばらつき（Stage 8）---
	// 弾の向き = 砲塔の向き + ランダムなずれ。
	// ずれは「正規分布（ガウス分布）」から選ぶ。中心付近がよく出て、端はたまにしか出ない。
	//   一様分布（Random(-m_spread, m_spread)）だと、端も中心も同じ確率で出るので「運ゲー」に感じやすい。
	// std::normal_distribution<double>{ 平均, 標準偏差 } は C++ 標準の <random> にある分布。
	// GetDefaultRNG() は Siv3D の乱数生成器（Random() が内部で使っているもの）。
	// 標準偏差を m_spread の半分にすると、約 95% のずれが ±m_spread の中に収まる。
	// 残りの 5% もはみ出さないよう、最後に Clamp で範囲内に収める。
	std::normal_distribution<double> distribution{ 0.0, (m_spread / 2.0) };
	const double error = Clamp(distribution(GetDefaultRNG()), -m_spread, m_spread);

	const double shotAngle = m_turretAngle + error;
	const Vec2 shotDirection{ Math::Cos(shotAngle), Math::Sin(shotAngle) };

	// emplace_back: 引数をそのまま Bullet のコンストラクタに渡して、配列の末尾に直接作る。
	// bullets.push_back(Bullet{ muzzlePos, shotDirection * BulletSpeed, m_team }); と同じ意味。
	// 弾には自分のチームを持たせる（同じチームの戦車には当たらない）
	bullets.emplace_back(muzzlePos, shotDirection * BulletSpeed, m_team);

	// 撃った反動で、ばらつきが広がる（連射するほど当たりにくくなる）
	m_spread = Min(m_spread + FireKick, MaxSpread);

	m_fireCooldown = FireInterval;
	m_firedThisFrame = true;
}

// ------------------------------------------------------------
// 描画
// ------------------------------------------------------------
// 宣言と同じく、定義にも末尾の const が必要。
void Tank::draw() const
{
	// 色はメンバ変数 m_color から作る（戦車ごとに色を変えられる）
	// lerp(相手の色, 割合) は2色の間の色を求める。黒に 30% 近づけると暗い色になる。
	const ColorF trackColor{ 0.25 };
	const ColorF darkColor = m_color.lerp(ColorF{ 0.0 }, 0.3);
	const ColorF turretColor = m_color.lerp(ColorF{ 1.0 }, 0.15);

	// --- 車体 ---
	{
		const Transformer2D transform{ Mat3x2::Rotate(m_bodyAngle).translated(m_pos) };

		// 履帯（キャタピラ）
		RectF{ Arg::center(0, -BodySize.y / 2), BodySize.x + 4, 8 }.draw(trackColor);
		RectF{ Arg::center(0, BodySize.y / 2), BodySize.x + 4, 8 }.draw(trackColor);

		// 車体本体
		RectF{ Arg::center(0, 0), BodySize }.draw(m_color);

		// 前方の目印
		RectF{ Arg::center(BodySize.x / 2 - 4, 0), 6, 20 }.draw(darkColor);
	}

	// --- 砲塔 ---
	{
		const Transformer2D transform{ Mat3x2::Rotate(m_turretAngle).translated(m_pos) };

		// 砲身
		RectF{ 0, -BarrelWidth / 2, BarrelLength, BarrelWidth }.draw(darkColor);

		// 砲塔の丸い部分
		Circle{ 0, 0, TurretRadius }.draw(turretColor);
	}
}
