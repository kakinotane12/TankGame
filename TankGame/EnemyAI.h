# pragma once
# include <Siv3D.hpp>
# include "Tank.h"
# include "Map.h"

// ============================================================
// EnemyAI: 敵の「頭脳」の基底クラス
// ============================================================
// 敵の戦車（Tank）そのものはプレイヤーと同じクラス。違うのは「誰が TankInput を作るか」だけ。
// その「TankInput を作る係」を EnemyAI として切り出し、種類ごとに派生クラスを作る。
//
//   EnemyAI（基底クラス：共通の窓口 think() を決める）
//     ├─ ChaserAI（プレイヤーを探して追いかける）
//     └─ SentryAI（動かずに見張り、見えたら撃つ）
//
// 使う側（Main）は「EnemyAI の think() を呼ぶ」ことだけを知っていればよく、
// 中身がどの AI なのかを気にしなくてよい（これを「ポリモーフィズム（多態性）」という）。
class EnemyAI
{
public:
	// 基底クラスのデストラクタは virtual にする（理由は docs/Stage5.md を参照）
	virtual ~EnemyAI() = default;

	// 毎フレーム呼ばれ、この敵への操作指示を返す。
	// = 0 は「純粋仮想関数」: 基底クラスでは中身を書かず、派生クラスに必ず書かせる。
	virtual TankInput think(const Tank& self, const Tank& player, const Map& map, double deltaTime) = 0;

	// 今の状態の名前（デバッグ表示用）
	virtual String getStateName() const = 0;

protected:
	// ---- protected: 派生クラスからは使えるが、外（Main など）からは使えない ----
	// どの AI でも使う計算を、ここにまとめておく。

	// 今の角度 currentAngle から、from → to の方向へ向くための回転指示（-1.0 〜 +1.0）を返す。
	// 角度の差が小さいほど小さな値を返すので、目標の向きの近くで左右に震えない。
	static double RotateToward(double currentAngle, const Vec2& from, const Vec2& to);

	// 砲塔が target の方を向いているか（角度の差が tolerance 以内か）
	static bool IsAimedAt(const Tank& self, const Vec2& target, double tolerance);

	// プレイヤーが見えるか（生きていて、range 以内の距離にいて、間に壁がない）
	static bool CanSeePlayer(const Tank& self, const Tank& player, const Map& map, double range);

	// 発見してから撃てるようになるまでの残り時間 [秒]（protected メンバ変数: 派生クラスから読み書きできる）
	// 見つけた瞬間に撃たれると理不尽なので、少しだけ「気づいてから構える」時間を置く。
	double m_reactionTimer = 0.0;
};

// ============================================================
// ChaserAI: 状態機械で動く追跡型の AI
// ============================================================
//   Patrol（巡回）  … ランダムな方向にうろうろする
//      │ プレイヤーが見えた
//      ▼
//   Chase（追跡）   … プレイヤーに近づき、狙いが合ったら撃つ
//      │ 見失った
//      ▼
//   Search（捜索）  … 最後に見た場所へ向かう。見つけたら Chase、時間切れなら Patrol
class ChaserAI : public EnemyAI
{
public:
	// override: 「基底クラスの virtual 関数を上書きしている」という印。
	// 名前や引数を間違えて上書きになっていないと、コンパイルエラーで教えてくれる。
	TankInput think(const Tank& self, const Tank& player, const Map& map, double deltaTime) override;

	String getStateName() const override;

private:
	enum class State
	{
		Patrol,
		Chase,
		Search,
	};

	State m_state = State::Patrol;

	// Patrol 用
	double m_patrolHeading = 0.0; // 進みたい方向 [ラジアン]
	double m_patrolTimer = 0.0;   // 次に方向を変えるまでの時間 [秒]

	// Chase / Search 用
	Vec2 m_lastSeenPos{ 0, 0 };   // 最後にプレイヤーを見た位置
	double m_searchTimer = 0.0;   // 捜索をあきらめるまでの時間 [秒]

	// 状態ごとの行動
	TankInput patrol(const Tank& self, const Map& map, double deltaTime);
	TankInput chase(const Tank& self, const Tank& player) const;
	TankInput search(const Tank& self) const;
};

// ============================================================
// SentryAI: 動かない固定砲台
// ============================================================
// 砲塔をぐるぐる回して見張り、プレイヤーが見えたら狙って撃つ。
class SentryAI : public EnemyAI
{
public:
	TankInput think(const Tank& self, const Tank& player, const Map& map, double deltaTime) override;

	String getStateName() const override;

private:
	double m_scanAngle = 0.0;  // 見張りで向けている方向 [ラジアン]
	bool m_alert = false;      // プレイヤーを発見しているか
};

// 敵の種類に応じた AI を作る
// std::unique_ptr<EnemyAI>: 「EnemyAI（の派生クラス）を1つだけ所有するポインタ」。
// 所有者がいなくなると、中身も自動で delete される。
std::unique_ptr<EnemyAI> MakeEnemyAI(EnemyType type);
