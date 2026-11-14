# pragma once
// ↑ インクルードガード。このヘッダーが同じ .cpp に何度 #include されても、中身を1回だけ読み込む。
//   これがないと「Tank が二重に定義されている」というコンパイルエラーになる。

# include <Siv3D.hpp>
# include "Bullet.h" // Tank は弾を生み出すので、Bullet を知っている必要がある
# include "Map.h"    // 壁との当たり判定に使う（Bullet.h 経由でも読み込まれるが、使うものは自分でインクルードしておく）
# include "Team.h"

//
// ヘッダー（.h）には「宣言」を書く。
//   = このクラスに何があるか（メンバ変数・関数の一覧）。他のファイルが Tank を使うときに見る「説明書」。
// 関数の中身（定義）は Tank.cpp に書く。
//

// ------------------------------------------------------------
// TankInput: 戦車への「操作指示」
// ------------------------------------------------------------
// struct はメンバがデフォルトで public（外から自由に読み書きできる）。
// 「ただのデータの入れ物」には struct、「守るべきルールを持つもの」には class を使うのが慣習。
//
// Tank はキーボードを直接読まず、この指示を受け取って動く。
// こうしておけば、プレイヤー（キーボード）でも敵（AI）でも同じ Tank を操作できる。
struct TankInput
{
	// 前後移動: +1 = 前進, -1 = 後退, 0 = 停止
	double move = 0.0;

	// 車体の回転: +1 = 時計回り, -1 = 反時計回り, 0 = 回転しない
	double rotate = 0.0;

	// 砲塔を向けたい位置（画面上の座標）
	Vec2 aimTarget{ 0, 0 };

	// 撃ちたいか。true でも、クールダウン中なら撃たれない（判断するのは Tank）。
	bool fire = false;
};

// ------------------------------------------------------------
// Tank: 戦車1台ぶんの状態と振る舞い
// ------------------------------------------------------------
class Tank
{
public:
	// ---- public: クラスの外から使ってよいもの ----

	// コンストラクタ: Tank を作るときに1回だけ呼ばれる初期化用の関数。
	// 戻り値を書かず、名前はクラス名と同じにする。
	Tank(const Vec2& pos, const ColorF& color, Team team);

	// 1フレーム分、状態を進める（入力を受けて移動・回転し、撃てるなら弾を撃つ）
	// map: 壁との当たり判定に使う。読むだけなので const 参照。
	// bullets: 撃った弾を追加する先。
	//   「const なしの参照（&）」で受け取るので、関数の中で追加した弾が呼び出し元の配列に残る。
	//   もし参照なし（Array<Bullet> bullets）にすると、コピーに追加されるだけで、弾は消えてしまう。
	void update(const TankInput& input, double deltaTime, const Map& map, Array<Bullet>& bullets);

	// 現在の状態を描画する。
	// 末尾の const = 「この関数はメンバ変数を書き換えない」という約束。
	// draw() の中でうっかり m_pos を変えようとすると、コンパイルエラーになる。
	void draw() const;

	// ゲッター: private なメンバ変数を「読むだけ」許可する関数。
	// const 参照（const Vec2&）で返すと、コピーせずに渡せて、しかも書き換えられない。
	const Vec2& getPos() const { return m_pos; }
	double getBodyAngle() const { return m_bodyAngle; }
	double getTurretAngle() const { return m_turretAngle; }

	Team getTeam() const { return m_team; }

	// 当たり判定用の半径 [ピクセル]（車体を円とみなす）
	static constexpr double CollisionRadius = 20.0;

	// 当たり判定用の円
	Circle getCircle() const { return Circle{ m_pos, CollisionRadius }; }

	// 撃破されていないか
	bool isAlive() const { return m_alive; }

	// 弾が当たったときに呼ぶ（このゲームでは1発で撃破）
	void destroy() { m_alive = false; }

private:
	// ---- private: クラスの中だけで使うもの ----

	// update() の中身を役割ごとに分けた補助関数（外から呼ぶ必要はないので private）
	void updateBody(const TankInput& input, double deltaTime, const Map& map);
	void updateTurret(const Vec2& aimTarget, double deltaTime);
	void updateFire(bool fire, double deltaTime, const Map& map, Array<Bullet>& bullets);

	// メンバ変数。名前の先頭の m_ は「メンバ変数」の目印（Siv3D でも使われている慣習）。
	// ローカル変数や引数と区別しやすくなる。
	Vec2 m_pos;                 // 中心の位置 [ピクセル]
	double m_bodyAngle = 0.0;   // 車体の向き [ラジアン]
	double m_turretAngle = 0.0; // 砲塔の向き [ラジアン]
	ColorF m_color;             // 車体の色
	double m_fireCooldown = 0.0; // 次に撃てるまでの残り時間 [秒]。0 なら撃てる。
	Team m_team;                 // どちらの陣営か（撃った弾にも引き継がれる）
	bool m_alive = true;         // 撃破されていないか
};
