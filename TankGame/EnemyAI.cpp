# include "EnemyAI.h"

namespace
{
	// ---- ChaserAI の調整値 ----
	constexpr double ChaserSightRange = 400.0;  // 見える距離 [ピクセル]
	constexpr double ChaserKeepDistance = 150.0; // これより近づいたら止まる [ピクセル]
	constexpr double ChaserSearchTime = 5.0;     // 見失ってから捜索する時間 [秒]
	constexpr double PatrolSpeed = 0.6;          // 巡回中の移動の強さ（0.0 〜 1.0）

	// ---- SentryAI の調整値 ----
	constexpr double SentrySightRange = 500.0;   // 見える距離 [ピクセル]
	constexpr double SentryScanSpeed = 30_deg;   // 見張り中に砲塔を回す速さ [ラジアン/秒]

	// ---- 共通 ----
	constexpr double FireTolerance = 5_deg;      // 砲塔の向きがこの角度以内に合ったら撃つ
	constexpr double ReactionTime = 0.6;         // 発見してから撃てるようになるまでの時間 [秒]

	// 角度 → その方向を向いた長さ 1 のベクトル
	Vec2 Direction(const double angle)
	{
		return Vec2{ Math::Cos(angle), Math::Sin(angle) };
	}

	// from から to への角度
	double AngleTo(const Vec2& from, const Vec2& to)
	{
		const Vec2 v = (to - from);
		return Math::Atan2(v.y, v.x);
	}

	// 今の角度 current から目標の角度 target までの差（-π 〜 π）
	double AngleDiff(const double current, const double target)
	{
		return Math::NormalizeAngle((target - current), 0.0);
	}
}

// ============================================================
// EnemyAI（共通の補助関数）
// ============================================================
double EnemyAI::RotateToward(const double currentAngle, const Vec2& from, const Vec2& to)
{
	const double diff = AngleDiff(currentAngle, AngleTo(from, to));

	// 差が 15° 以上なら全力（±1.0）、それより小さければ差に比例して弱める。
	// Stage 2 の練習課題では ±1.0 しか使わなかったので、目標の近くで行き過ぎて左右に震えていた。
	return Clamp((diff / 15_deg), -1.0, 1.0);
}

bool EnemyAI::IsAimedAt(const Tank& self, const Vec2& target, const double tolerance)
{
	const double diff = AngleDiff(self.getTurretAngle(), AngleTo(self.getPos(), target));
	return (Math::Abs(diff) < tolerance);
}

bool EnemyAI::CanSeePlayer(const Tank& self, const Tank& player, const Map& map, const double range)
{
	if (not player.isAlive())
	{
		return false;
	}

	// distanceFrom: 2点間の距離
	if (range < self.getPos().distanceFrom(player.getPos()))
	{
		return false;
	}

	return map.canSee(self.getPos(), player.getPos());
}

// ============================================================
// ChaserAI
// ============================================================
TankInput ChaserAI::think(const Tank& self, const Tank& player, const Map& map, const double deltaTime)
{
	const bool seesPlayer = CanSeePlayer(self, player, map, ChaserSightRange);

	m_reactionTimer = Max(m_reactionTimer - deltaTime, 0.0);

	if (seesPlayer)
	{
		m_lastSeenPos = player.getPos();
	}

	// ---- 1. 状態を切り替える（遷移） ----
	switch (m_state)
	{
	case State::Patrol:
		if (seesPlayer)
		{
			m_state = State::Chase;
			m_reactionTimer = ReactionTime; // 見つけた直後は撃たない
		}
		break;

	case State::Chase:
		if (not seesPlayer)
		{
			m_state = State::Search;
			m_searchTimer = ChaserSearchTime;
		}
		break;

	case State::Search:
		m_searchTimer -= deltaTime;

		if (seesPlayer)
		{
			m_state = State::Chase;
			m_reactionTimer = ReactionTime;
		}
		else if ((m_searchTimer <= 0.0) || (self.getPos().distanceFrom(m_lastSeenPos) < 30.0))
		{
			// 時間切れ、または最後に見た場所に着いても見つからない → あきらめて巡回に戻る
			m_state = State::Patrol;
			m_patrolTimer = 0.0; // すぐに新しい方向を決めさせる
		}
		break;
	}

	// ---- 2. 今の状態に応じて行動する ----
	switch (m_state)
	{
	case State::Patrol:
		return patrol(self, map, deltaTime);

	case State::Chase:
		return chase(self, player);

	case State::Search:
		return search(self);
	}

	return TankInput{}; // ここには来ないが、何も返さないとコンパイラが警告するので書いておく
}

String ChaserAI::getStateName() const
{
	switch (m_state)
	{
	case State::Patrol:
		return U"Patrol";
	case State::Chase:
		return U"Chase";
	case State::Search:
		return U"Search";
	}

	return U"";
}

TankInput ChaserAI::patrol(const Tank& self, const Map& map, const double deltaTime)
{
	m_patrolTimer -= deltaTime;

	// 一定時間たったか、進みたい方向のすぐ先が壁なら、新しい方向をランダムに決める
	const Vec2 lookAhead = self.getPos() + Direction(m_patrolHeading) * (Tank::CollisionRadius + 30.0);

	if ((m_patrolTimer <= 0.0) || map.isWallAt(lookAhead))
	{
		// Random(a, b): a 以上 b 以下のランダムな値
		m_patrolHeading = Random(-180_deg, 180_deg);
		m_patrolTimer = Random(1.5, 3.5);
	}

	TankInput input;

	const Vec2 headingPoint = self.getPos() + Direction(m_patrolHeading);
	input.rotate = RotateToward(self.getBodyAngle(), self.getPos(), headingPoint);

	// 向きがだいたい合ってから進む（その場で回ってから走り出す）
	if (Math::Abs(AngleDiff(self.getBodyAngle(), m_patrolHeading)) < 30_deg)
	{
		input.move = PatrolSpeed;
	}

	// 砲塔は進行方向を向けておく
	input.aimTarget = self.getPos() + Direction(self.getBodyAngle()) * 100.0;

	return input;
}

TankInput ChaserAI::chase(const Tank& self, const Tank& player) const
{
	// Stage 2〜3 の練習課題で作った MakeChaseInput() の考え方をそのまま使っている
	TankInput input;

	const Vec2 target = player.getPos();
	const double distance = self.getPos().distanceFrom(target);
	const double bodyDiff = AngleDiff(self.getBodyAngle(), AngleTo(self.getPos(), target));

	input.rotate = RotateToward(self.getBodyAngle(), self.getPos(), target);

	// 車体の向きが合っていて、離れているなら近づく
	if ((Math::Abs(bodyDiff) < 10_deg) && (ChaserKeepDistance < distance))
	{
		input.move = 1.0;
	}

	input.aimTarget = target;

	// 反応時間が過ぎていて、砲塔の狙いが合ったら撃つ
	// （Chase 中はプレイヤーが見えているので、壁越しには撃たない）
	input.fire = ((m_reactionTimer <= 0.0) && IsAimedAt(self, target, FireTolerance));

	return input;
}

TankInput ChaserAI::search(const Tank& self) const
{
	// 最後に見た場所へ向かう
	TankInput input;

	const double bodyDiff = AngleDiff(self.getBodyAngle(), AngleTo(self.getPos(), m_lastSeenPos));

	input.rotate = RotateToward(self.getBodyAngle(), self.getPos(), m_lastSeenPos);

	if (Math::Abs(bodyDiff) < 20_deg)
	{
		input.move = 1.0;
	}

	input.aimTarget = m_lastSeenPos;

	return input;
}

// ============================================================
// SentryAI
// ============================================================
TankInput SentryAI::think(const Tank& self, const Tank& player, const Map& map, const double deltaTime)
{
	TankInput input; // move と rotate は 0 のまま（動かない）

	const bool wasAlert = m_alert;
	m_alert = CanSeePlayer(self, player, map, SentrySightRange);

	m_reactionTimer = Max(m_reactionTimer - deltaTime, 0.0);

	// 見えていなかった → 見えた、に変わった瞬間に反応時間を設定する
	if (m_alert && (not wasAlert))
	{
		m_reactionTimer = ReactionTime;
	}

	if (m_alert)
	{
		// 見えている: プレイヤーを狙い、反応時間が過ぎていて狙いが合ったら撃つ
		input.aimTarget = player.getPos();
		input.fire = ((m_reactionTimer <= 0.0) && IsAimedAt(self, player.getPos(), FireTolerance));

		// 見失ったときに、今の向きから見張りを続けられるようにしておく
		m_scanAngle = self.getTurretAngle();
	}
	else
	{
		// 見えていない: 砲塔をゆっくり回して見張る
		m_scanAngle += SentryScanSpeed * deltaTime;
		input.aimTarget = self.getPos() + Direction(m_scanAngle) * 100.0;
	}

	return input;
}

String SentryAI::getStateName() const
{
	// 三項演算子は String を返すこともできる
	return m_alert ? U"Alert" : U"Scan";
}

// ============================================================
// 生成
// ============================================================
std::unique_ptr<EnemyAI> MakeEnemyAI(const EnemyType type)
{
	switch (type)
	{
	case EnemyType::Chaser:
		// std::make_unique<T>(): T を new して unique_ptr に入れる（自分で new を書かない）
		return std::make_unique<ChaserAI>();

	case EnemyType::Sentry:
		return std::make_unique<SentryAI>();
	}

	return nullptr;
}
