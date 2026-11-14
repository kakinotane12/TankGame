# pragma once
# include <Siv3D.hpp>

// ------------------------------------------------------------
// シーン間で共有するデータ
// ------------------------------------------------------------
// シーン（Title / Game / Result）は切り替わるたびに作り直される。
// シーンのメンバ変数はそのたびに消えるので、「シーンをまたいで残したい値」はここに置く。
// どのシーンからも getData() で同じ GameData を読み書きできる。
struct GameData
{
	int32 stage = 1;        // 今のステージ番号（1 から）
	int32 lives = 3;        // 残機
	bool allCleared = false; // 全ステージをクリアしたか（Result 画面の表示に使う）
};

// シーンマネージャーの型に別名を付ける。
// SceneManager<シーンの名前の型, 共有データの型>
// using を使うと、長い型名を短い名前で書けるようになる。
using App = SceneManager<String, GameData>;

// ゲーム全体の設定
constexpr int32 StageCount = 3;   // ステージの数（map/stage1.txt 〜 map/stage3.txt）
constexpr int32 InitialLives = 3; // 最初の残機

// ステージ番号 → マップファイルのパス
// ヘッダーに関数の中身を書くときは inline を付ける（複数の .cpp からインクルードされても二重定義にならない）
inline FilePath StageMapPath(const int32 stage)
{
	return U"map/stage{}.txt"_fmt(stage);
}
