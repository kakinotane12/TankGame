# pragma once
# include <Siv3D.hpp>

// ------------------------------------------------------------
// Bullet: 弾1発ぶんの状態と振る舞い
// ------------------------------------------------------------
// 弾は「撃たれたら一定時間まっすぐ飛び、寿命が来たら消える」。
// 消すかどうかの判断は isAlive() で外に知らせ、実際に配列から取り除くのは持ち主（Main）の仕事。
class Bullet
{
public:
	// pos: 発射位置、velocity: 速度ベクトル（向き × 速さ）[ピクセル/秒]
	Bullet(const Vec2& pos, const Vec2& velocity);

	// 1フレーム分進める（移動して、寿命を減らす）
	void update(double deltaTime);

	void draw() const;

	// まだ生きているか（寿命が残っているか）
	bool isAlive() const;

	const Vec2& getPos() const { return m_pos; }

private:
	Vec2 m_pos;        // 位置 [ピクセル]
	Vec2 m_velocity;   // 速度 [ピクセル/秒]
	double m_lifeTime; // 残りの寿命 [秒]
};
