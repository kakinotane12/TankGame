# include "Map.h"

RectF Map::TileRect(const Point& tile)
{
	// マス目 (x, y) の左上のピクセル座標は (x * TileSize, y * TileSize)
	return RectF{ (tile.x * TileSize), (tile.y * TileSize), TileSize, TileSize };
}

// ------------------------------------------------------------
// 読み込み
// ------------------------------------------------------------
bool Map::load(const FilePathView path)
{
	// TextReader: テキストファイルを読むためのクラス。
	// ファイルが開けなかった場合、if (not reader) が true になる。
	TextReader reader{ path };

	if (not reader)
	{
		return false;
	}

	// まず全部の行を読み込む（マップの幅と高さを知るため）
	Array<String> lines;
	String line;

	while (reader.readLine(line))
	{
		lines << line; // << は Array の末尾に追加する（push_back と同じ）
	}

	if (lines.isEmpty())
	{
		return false;
	}

	// 幅 = いちばん長い行の文字数、高さ = 行数
	size_t width = 0;
	for (const auto& l : lines)
	{
		width = Max(width, l.size());
	}
	const size_t height = lines.size();

	// 全部「床」で埋めた width × height の Grid を作り、文字に応じて書き換えていく
	m_tiles = Grid<TileType>(width, height, TileType::Floor);
	m_wallRects.clear();
	m_enemyStarts.clear();

	bool hasPlayerStart = false;

	for (size_t y = 0; y < height; ++y)
	{
		for (size_t x = 0; x < lines[y].size(); ++x)
		{
			const Point tile{ static_cast<int32>(x), static_cast<int32>(y) };

			// switch: 値によって処理を分ける。char32（Siv3D の文字型）は U'#' のように書く。
			switch (lines[y][x])
			{
			case U'#':
				m_tiles[tile] = TileType::Wall;
				m_wallRects << TileRect(tile);
				break; // break を忘れると、次の case まで続けて実行されてしまう

			case U'P':
				m_playerStart = TileRect(tile).center();
				hasPlayerStart = true;
				break;

			case U'E':
				m_enemyStarts << TileRect(tile).center();
				break;

			default: // それ以外の文字（'.' など）は床のまま
				break;
			}
		}
	}

	// プレイヤーの初期位置がないマップは不正
	return hasPlayerStart;
}

// ------------------------------------------------------------
// 描画
// ------------------------------------------------------------
void Map::draw() const
{
	const ColorF floorColorA{ 0.85, 0.80, 0.65 };
	const ColorF floorColorB{ 0.82, 0.77, 0.62 };
	const ColorF wallColor{ 0.45, 0.40, 0.35 };
	const ColorF wallTopColor{ 0.55, 0.50, 0.44 };

	for (int32 y = 0; y < static_cast<int32>(m_tiles.height()); ++y)
	{
		for (int32 x = 0; x < static_cast<int32>(m_tiles.width()); ++x)
		{
			const Point tile{ x, y };
			const RectF rect = TileRect(tile);

			if (m_tiles[tile] == TileType::Wall)
			{
				rect.draw(wallColor);
				rect.stretched(-4).draw(wallTopColor); // stretched(-4): 4px 内側に縮めた長方形
			}
			else
			{
				// 床は市松模様にして、マス目を分かりやすくする
				rect.draw(((x + y) % 2 == 0) ? floorColorA : floorColorB);
			}
		}
	}
}

// ------------------------------------------------------------
// 当たり判定
// ------------------------------------------------------------
bool Map::intersectsWall(const Circle& circle) const
{
	for (const auto& rect : m_wallRects)
	{
		if (circle.intersects(rect))
		{
			return true;
		}
	}

	return false;
}

bool Map::isWallAt(const Vec2& pos) const
{
	// ピクセル座標 → マス目の座標（TileSize で割って切り捨て）
	const Point tile{ static_cast<int32>(Math::Floor(pos.x / TileSize)), static_cast<int32>(Math::Floor(pos.y / TileSize)) };

	// マップの外は壁として扱う
	if (not m_tiles.inBounds(tile))
	{
		return true;
	}

	return (m_tiles[tile] == TileType::Wall);
}

Vec2 Map::pushOut(const Vec2& center, const double radius) const
{
	Vec2 result = center;

	for (const auto& rect : m_wallRects)
	{
		// 長方形の中で、円の中心にいちばん近い点を求める。
		// 中心の x, y をそれぞれ長方形の範囲に Clamp するだけで求まる。
		const Vec2 closest{
			Clamp(result.x, rect.x, (rect.x + rect.w)),
			Clamp(result.y, rect.y, (rect.y + rect.h)) };

		const Vec2 away = (result - closest); // 壁から円の中心へ向かうベクトル
		const double distance = away.length();

		// 距離が半径より小さい = めり込んでいる。
		// めり込んだ分（radius - distance）だけ、壁から離れる向きに押し戻す。
		// （distance が 0 = 中心が壁の中、のときは向きが決められないので何もしない）
		if ((0.0 < distance) && (distance < radius))
		{
			result += away.normalized() * (radius - distance);
		}
	}

	return result;
}
