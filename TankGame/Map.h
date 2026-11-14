# pragma once
# include <Siv3D.hpp>

// ------------------------------------------------------------
// TileType: マップの1マスの種類
// ------------------------------------------------------------
// enum class は「決まった選択肢のどれか」を表す型。
// int で 0 = 床, 1 = 壁 のように表すこともできるが、
//   - TileType::Wall と書けば意味が一目で分かる
//   - 間違って 2 や -1 のような値を入れられない
//   - 他の enum class の値と混ざらない（TileType::Wall と Team::Player は比較できない）
// という利点がある。
enum class TileType
{
	Floor, // 床（通れる）
	Wall,  // 壁（通れない、弾が跳ね返る）
};

// ------------------------------------------------------------
// Map: タイルでできたマップ
// ------------------------------------------------------------
// テキストファイルから読み込む。1文字が1マス:
//   '#' = 壁, '.' = 床, 'P' = プレイヤーの初期位置, 'E' = 敵の初期位置
class Map
{
public:
	// 1マスの大きさ [ピクセル]
	// static constexpr メンバ: クラスに1つだけある定数。Map::TileSize と書いて外からも使える。
	static constexpr double TileSize = 40.0;

	// テキストファイルからマップを読み込む。成功したら true を返す。
	bool load(FilePathView path);

	void draw() const;

	// 円が壁に重なっているか（弾の跳ね返り判定に使う）
	bool intersectsWall(const Circle& circle) const;

	// 点が壁の中か（マップの外も壁として扱う）
	bool isWallAt(const Vec2& pos) const;

	// 円（中心 center, 半径 radius）が壁にめり込んでいたら、押し出した後の中心位置を返す
	Vec2 pushOut(const Vec2& center, double radius) const;

	const Vec2& getPlayerStart() const { return m_playerStart; }
	const Array<Vec2>& getEnemyStarts() const { return m_enemyStarts; }

private:
	// Grid<T> は Siv3D の2次元配列。m_tiles[y][x] または m_tiles[Point{ x, y }] で読み書きする。
	Grid<TileType> m_tiles;

	// 壁のマスの長方形を、読み込み時にまとめて作っておく（当たり判定で毎回作り直さないため）
	Array<RectF> m_wallRects;

	Vec2 m_playerStart{ 0, 0 };
	Array<Vec2> m_enemyStarts;

	// マス目の座標 (x, y) → そのマスの長方形（画面上のピクセル座標）
	static RectF TileRect(const Point& tile);
};
