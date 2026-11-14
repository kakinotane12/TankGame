# include "Bullet.h"

namespace
{
	// 弾の半径 [ピクセル]
	constexpr double BulletRadius = 5.0;

	// 弾の寿命 [秒]。跳ね返りながら飛び続けても、この時間が過ぎると消える。
	constexpr double BulletLifeTime = 5.0;

	// 跳ね返れる回数。これを超えて壁に当たると消える。
	constexpr int32 MaxBounces = 1;
}

Bullet::Bullet(const Vec2& pos, const Vec2& velocity, const Team team)
	: m_pos{ pos }
	, m_velocity{ velocity }
	, m_team{ team }
	, m_lifeTime{ BulletLifeTime }
{
}

bool Bullet::isAlive() const
{
	// Scene::Rect() は画面全体の長方形。マップの外周は壁なので、通常は画面外に出ることはないが、念のため残す。
	return ((not m_destroyed)
		&& (0.0 < m_lifeTime)
		&& (m_bounceCount <= MaxBounces)
		&& Scene::Rect().contains(m_pos));
}

Circle Bullet::getCircle() const
{
	return Circle{ m_pos, BulletRadius };
}

void Bullet::update(const double deltaTime, const Map& map)
{
	// --- x 方向と y 方向を別々に動かして、跳ね返りを判定する ---
	// 壁は縦か横の辺しか持たない（タイルなので）。
	//   x 方向に動いて壁に当たった → 縦の壁に当たった → 速度の x 成分を反転
	//   y 方向に動いて壁に当たった → 横の壁に当たった → 速度の y 成分を反転
	// こうすると、角に斜めに当たったときも自然に跳ね返る。

	// x 方向
	m_pos.x += m_velocity.x * deltaTime;
	if (map.intersectsWall(Circle{ m_pos, BulletRadius }))
	{
		m_pos.x -= m_velocity.x * deltaTime; // 壁に入る前の位置に戻す
		m_velocity.x = -m_velocity.x;        // 反射
		++m_bounceCount;
	}

	// y 方向
	m_pos.y += m_velocity.y * deltaTime;
	if (map.intersectsWall(Circle{ m_pos, BulletRadius }))
	{
		m_pos.y -= m_velocity.y * deltaTime;
		m_velocity.y = -m_velocity.y;
		++m_bounceCount;
	}

	// 寿命を減らす。0 以下になったら isAlive() が false を返す。
	m_lifeTime -= deltaTime;
}

void Bullet::draw() const
{
	// プレイヤーの弾は黒、敵の弾は赤っぽくして、どちらの弾か見分けられるようにする
	const ColorF color = (m_team == Team::Player) ? ColorF{ 0.15 } : ColorF{ 0.55, 0.10, 0.05 };
	getCircle().draw(color);
}
