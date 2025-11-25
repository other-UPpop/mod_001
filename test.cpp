#include <dpp/dpp.h>        // D++ (Discord++) ライブラリのヘッダーファイルをインクルード
#include <iostream>       // 標準入出力（cout, cerrなど）を使用するためにインクルード
#include <map>            // ユーザーの通話参加時間を記録するためのstd::mapをインクルード
#include <chrono>         // 時間計測（タイムスタンプ取得、期間計算）のためにインクルード

// Botのトークンを定義。**必ずここを自身のトークンに置き換える必要があります。**
const std::string BOT_TOKEN = "YOUR_BOT_TOKEN_HERE";

// ユーザーID（dpp::snowflake）をキー、通話参加時刻（std::chrono::time_point）を値とするマップを定義
// これが通話時間計測のデータベースとして機能します。
std::map<dpp::snowflake, std::chrono::time_point<std::chrono::system_clock>> voice_join_times;

// --- 通話時間の表示を整形するためのヘルパー関数 ---

// 秒数を「h時間 m分 s秒」の形式に変換する関数
std::string format_duration(long long total_seconds) {
    if (total_seconds < 0) total_seconds = 0; // マイナス値のガード
    long long hours = total_seconds / 3600;
    long long minutes = (total_seconds % 3600) / 60;
    long long seconds = total_seconds % 60;

    std::string result;
    if (hours > 0) {
        result += std::to_string(hours) + "時間 ";
    }
    if (minutes > 0 || hours > 0) { // 1時間未満でも分があれば表示
        result += std::to_string(minutes) + "分 ";
    }
    result += std::to_string(seconds) + "秒";
    return result;
}


int main() {
    dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content | dpp::i_voice_states);

    // ログイベントの購読: Botの起動やエラーなどの情報をコンソールに出力
    bot.on_log([&bot](const dpp::log_t& event) {
        // INFOレベル以上のログのみを出力し、DEBUGログなどを省略
        if (event.severity > dpp::ll_info) {
            std::cout << dpp::utility::loglevel(event.severity) << ": " << event.message << "\n";
        }
    });

    bot.on_message_create([&bot](const dpp::message_create_t& event) {
        if (event.msg.author.is_bot()) {
            return;
        }

        bot.message_add_reaction(
            event.msg.id,
            event.msg.channel_id,
            "👍",
            [&bot](const dpp::confirmation_callback_t& cc) {
                // リアクション追加に失敗した場合のエラー処理（例：権限不足）
                if (cc.is_error()) {
                    std::cerr << "Reaction failed: " << cc.get_error().message << "\n";
                }
            }
        );
    });

    // 2. --- ボイスステータス更新イベントの処理（通話時間集計機能） ---
    // ユーザーがボイスチャンネルに参加/退出/移動したときに発火
    bot.on_voice_state_update([&bot](const dpp::voice_state_update_t& event) {
        dpp::snowflake user_id = event.state.user_id;       // ステータスを更新したユーザーのID
        dpp::snowflake channel_id = event.state.channel_id; // 現在接続しているボイスチャンネルのID

        // 退出時（channel_idが0、つまりどのボイスチャンネルにも接続していない場合）
        if (channel_id == 0) {
            // マップからユーザーの参加記録を検索
            auto it = voice_join_times.find(user_id);
            if (it != voice_join_times.end()) { // 記録がある場合
                
                auto join_time = it->second;                      // 参加時刻を取得
                auto leave_time = std::chrono::system_clock::now(); // 現在時刻（退出時刻）を取得
                
                // 滞在時間（期間）を計算し、秒単位に変換
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(leave_time - join_time);
                long long total_seconds = duration.count();
                std::string duration_str = format_duration(total_seconds); // 見やすい形式に整形

                // 通話時間の記録をマップから削除
                voice_join_times.erase(it);

                // --- 結果の出力 ---
                dpp::guild* g = dpp::find_guild(event.state.guild_id); // ギルド（サーバー）情報を取得
                if (g) {
                    // ギルドのシステムチャンネル（Botがメッセージを投げるチャンネルの例）を取得
                    dpp::channel* c = dpp::find_channel(g->system_channel_id); 
                    if (c) {
                        // 結果メッセージを作成: ユーザーメンションと通話時間を含める
                        dpp::message msg("通話リザルト: <@" + std::to_string(user_id) + "> さんの通話時間は **" + duration_str + "** でした。");
                        // システムチャンネルにメッセージを送信
                        bot.message_create(c->id, msg);
                    }
                }
            }
        } 
        // 参加時または移動時（channel_idが0でない場合）
        else {
            // 既に記録がある場合は（移動と見なし）古い記録を削除
            if (voice_join_times.count(user_id)) {
                voice_join_times.erase(user_id);
            }
            // 新しいチャンネルへの参加時刻を記録（現在時刻）
            voice_join_times[user_id] = std::chrono::system_clock::now();
        }
    });

    // Botを起動し、Botが終了するまでプログラムをブロックして待ち続ける
    bot.start(dpp::st_wait);
    
    return 0; // プログラム終了
}
