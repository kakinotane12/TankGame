# pragma once
# include <Siv3D.hpp>
# include "Map.h"
# include "Team.h"

// ------------------------------------------------------------
// Bullet: 弾1発ぶんの状態と振る舞い
// ------------------------------------------------------------
// 弾は「撃たれたらまっすぐ飛び、壁で跳ね返り、一定回数跳ね返るか寿命が来たら消える」。
// 戦車に当たったときも消える（当たったかどうかを調べるのは Main、弾は destroy() で消える印をつけられる）。
// 消すかどうかの判断は isAlive() で外に知らせ、実際に配列から取り除くのは持ち主（Main）の仕事。
class Bullet
{
public:
	// pos: 発射位置、velocity: 速度ベクトル（向き × 速さ）[ピクセル/秒]、team: 撃った戦車のチーム
	Bullet(const Vec2& pos, const Vec2& velocity, Team team);

	// 1フレーム分進める（移動して、壁に当たったら跳ね返り、寿命を減らす）
	// map は読むだけなので const 参照で受け取る
	void update(double deltaTime, const Map& map);

	void draw() const;

	// まだ生きているか（寿命が残っていて、跳ね返り回数が上限以内で、画面内にいて、何かに当たっていないか）
	bool isAlive() const;

	// 戦車に当たったときに呼ぶ。次の remove_if で取り除かれる。
	void destroy() { m_destroyed = true; }

	const Vec2& getPos() const { return m_pos; }
	Team getTeam() const { return m_team; }

	// 当たり判定用の円
	Circle getCircle() const;

	// このフレームの update() で壁に当たって跳ね返ったか（火花の演出・効果音に使う）
	bool bouncedThisFrame() const { return m_bouncedThisFrame; }

private:
	Vec2 m_pos;              // 位置 [ピクセル]
	Vec2 m_velocity;         // 速度 [ピクセル/秒]
	Team m_team;             // 撃った戦車のチーム
	double m_lifeTime;       // 残りの寿命 [秒]
	int32 m_bounceCount = 0; // 壁で跳ね返った回数
	bool m_destroyed = false; // 何かに当たって消えることになったか
	bool m_bouncedThisFrame = false; // このフレームで跳ね返ったか
};
