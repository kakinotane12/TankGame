# include "Bullet.h"

namespace
{
	// 弾の半径 [ピクセル]
	constexpr double BulletRadius = 5.0;

	// 弾の寿命 [秒]。この時間が過ぎると消える。
	// （Stage 4 で壁を作るまでは、画面外に飛んでいった弾もこの寿命で消える）
	constexpr double BulletLifeTime = 3.0;
}

Bullet::Bullet(const Vec2& pos, const Vec2& velocity)
	: m_pos{ pos }
	, m_velocity{ velocity }
	, m_lifeTime{ BulletLifeTime }
{
}

bool Bullet::isAlive() const 
{ 
	// Scene::Rectは画面全体の長方形を表す
	// Scene::Rect().contains(座標)で点が画面内にあるか調べられる
	return (0.0 < m_lifeTime &&  Scene::Rect().contains(m_pos)); 
}

void Bullet::update(const double deltaTime)
{
	// 速度 × 時間 = 移動量（Stage 1 の戦車の移動と同じ考え方）
	m_pos += m_velocity * deltaTime;

	// 寿命を減らす。0 以下になったら isAlive() が false を返す。
	m_lifeTime -= deltaTime;
}

void Bullet::draw() const
{
	Circle{ m_pos, BulletRadius }.draw(ColorF{ 0.15 });
}
