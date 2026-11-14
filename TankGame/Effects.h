# pragma once
# include <Siv3D.hpp>

//
// 演出（エフェクト）
//
// Siv3D の Effect は「時間がたつと消える描画」をまとめて管理する入れ物。
//   effect.add(...) で追加し、毎フレーム effect.update() を呼ぶと、追加されたものが描かれる。
//   それぞれの演出は「経過時間 t [秒] を受け取って描き、まだ続けるなら true を返す関数」として作る。
//
// 作り方は2通りある:
//   1. IEffect を継承したクラス（内部に状態をたくさん持つ演出向け）     … ExplosionEffect
//   2. ラムダ式（短い演出向け）                                        … AddMuzzleFlash / AddBounceSpark
//

// ------------------------------------------------------------
// 爆発（戦車が撃破されたとき）
// ------------------------------------------------------------
struct ExplosionEffect : IEffect
{
	// 破片1つぶんの情報
	struct Debris
	{
		Vec2 direction; // 飛ぶ向き（長さ 1）
		double distance; // 最終的に飛ぶ距離 [ピクセル]
		double size;     // 大きさ [ピクセル]
	};

	Vec2 m_pos;
	ColorF m_color;
	Array<Debris> m_debris;

	ExplosionEffect(const Vec2& pos, const ColorF& color);

	// t: この演出が始まってからの時間 [秒]。false を返すと、この演出は Effect から取り除かれる。
	bool update(double t) override;
};

// 発射時の閃光（砲口の位置と向きを受け取る）
void AddMuzzleFlash(const Effect& effect, const Vec2& pos, double angle);

// 弾が壁で跳ね返ったときの火花
void AddBounceSpark(const Effect& effect, const Vec2& pos);
