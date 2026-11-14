# include "Effects.h"

namespace
{
	constexpr double ExplosionDuration = 0.8;  // 爆発の長さ [秒]
	constexpr double MuzzleFlashDuration = 0.08;
	constexpr double SparkDuration = 0.15;
}

// ------------------------------------------------------------
// 爆発
// ------------------------------------------------------------
ExplosionEffect::ExplosionEffect(const Vec2& pos, const ColorF& color)
	: m_pos{ pos }
	, m_color{ color }
{
	// 破片をランダムに 12 個作る。作るのは最初の1回だけで、あとは時間に応じて位置を計算する。
	for (int32 i = 0; i < 12; ++i)
	{
		const double angle = Random(0.0, 360_deg);
		m_debris << Debris{ Vec2{ Math::Cos(angle), Math::Sin(angle) }, Random(30.0, 70.0), Random(3.0, 7.0) };
	}
}

bool ExplosionEffect::update(const double t)
{
	// 0.0（始まり）→ 1.0（終わり）の進み具合
	const double progress = (t / ExplosionDuration);

	// EaseOutCubic: 最初は速く、だんだんゆっくりになる変化（飛び散ってから減速する感じ）
	const double eased = EaseOutCubic(progress);

	// --- 閃光（最初の一瞬だけ明るい円） ---
	// （透明度は前半の 0.5 で 0 になる。マイナスにならないよう Max で 0 以上にする）
	Circle{ m_pos, (30.0 * (1.0 - progress)) }.draw(ColorF{ 1.0, 0.9, 0.6, Max((1.0 - progress * 2.0), 0.0) });

	// --- 衝撃波（広がっていく輪） ---
	Circle{ m_pos, (10.0 + 60.0 * eased) }.drawFrame((6.0 * (1.0 - progress)), ColorF{ 1.0, 0.6, 0.2, (1.0 - progress) });

	// --- 破片 ---
	for (const auto& d : m_debris)
	{
		const Vec2 pos = m_pos + d.direction * (d.distance * eased);
		RectF{ Arg::center(pos), d.size }.rotated(t * 10.0).draw(m_color.lerp(ColorF{ 0.1 }, progress).withAlpha(1.0 - progress));
	}

	// まだ続けるなら true
	return (t < ExplosionDuration);
}

// ------------------------------------------------------------
// 発射時の閃光（ラムダ式で作る）
// ------------------------------------------------------------
void AddMuzzleFlash(const Effect& effect, const Vec2& pos, const double angle)
{
	// [pos, angle] はキャプチャ: ラムダ式の外の変数を、ラムダ式の中にコピーして持ち込む。
	// この関数が終わった後も演出は続くので、「その時点の値のコピー」を持たせる必要がある。
	effect.add([pos, angle](const double t)
		{
			const double progress = (t / MuzzleFlashDuration);
			const ColorF color{ 1.0, 0.85, 0.4, (1.0 - progress) };

			// 砲口から前方に伸びる細長い光と、小さな円
			const Transformer2D transform{ Mat3x2::Rotate(angle).translated(pos) };
			Ellipse{ 8, 0, (14 * (1.0 - progress)), (5 * (1.0 - progress)) }.draw(color);
			Circle{ 0, 0, (7 * (1.0 - progress)) }.draw(color);

			return (t < MuzzleFlashDuration);
		});
}

// ------------------------------------------------------------
// 跳ね返りの火花（ラムダ式で作る）
// ------------------------------------------------------------
void AddBounceSpark(const Effect& effect, const Vec2& pos)
{
	// 火花の向きはランダムに決める。ラムダ式の中で毎フレーム Random を呼ぶと向きが毎回変わってしまうので、
	// 先に決めておいてキャプチャする（Array もコピーで持ち込める）。
	Array<double> angles;
	for (int32 i = 0; i < 6; ++i)
	{
		angles << Random(0.0, 360_deg);
	}

	effect.add([pos, angles](const double t)
		{
			const double progress = (t / SparkDuration);

			for (const double a : angles)
			{
				const Vec2 dir{ Math::Cos(a), Math::Sin(a) };
				const Vec2 from = pos + dir * (4.0 + 12.0 * progress);
				const Vec2 to = from + dir * (6.0 * (1.0 - progress));
				Line{ from, to }.draw(2.0, ColorF{ 1.0, 0.9, 0.5, (1.0 - progress) });
			}

			return (t < SparkDuration);
		});
}
