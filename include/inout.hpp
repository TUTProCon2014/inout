#pragma once

/**
このライブラリは、ネットワーク経由で問題を取得したりするものです。
*/
#include <cstdlib>
#include <type_traits>
#include <memory>
#include <boost/optional.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <array>
#include <string>
#include "../../utils/include/constants.hpp"
#include "../../utils/include/image.hpp"

namespace procon{ namespace inout {


bool download_ppm(std::string const & url, std::string const & save_file)
{
    auto curl_command = "curl -s -L -o " + save_file + " " + url;
    // curl
    if(std::system(curl_command.c_str()) == 0)
        return true;
    else
        return false;
}


std::string url_encode(std::string const & str)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for(auto& c: str){
        if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            escaped << c;
        else if(c == ' ')
            escaped << '+';
        else
            escaped << '%' << std::setw(2) << static_cast<int>(c) << std::setw(0);
    }

    return escaped.str();
}


bool curl_post(std::string const & url, std::vector<std::array<std::string, 2>> const & args)
{
    std::string command = "curl -s -X POST";
    for(auto& s : args)
        command += " -d " + s[0] + "=" + url_encode(s[1]);

    if(std::system((command + " " + url).c_str()) == 0)
        return true;
    else
        return false;
}


/** サーバーから送られてきた.ppmファイルの情報からオブジェクトを構築します
* arguments:
    * problem_data    = サーバーから送られてきた.ppmファイルへのパス


* return:
    読み込みに成功した場合はオブジェクトが返りますが、失敗した場合にはnull_opt()が返ります。
*/
boost::optional<utils::Problem> get_problem(std::string const & server_address, std::string const & problem_number)
{
    const auto url = "http://" + server_address + "/problem/prob" + problem_number + ".ppm",
               ppmFileName = "img" + problem_number +  ".ppm";

    if(download_ppm(url, ppmFileName))
        return utils::Problem::get(ppmFileName);
    else
        return boost::optional<utils::Problem>(boost::none);
}


/** テストサーバー(練習場)から問題番号idな画像を落とし、Problemオブジェクトを構築します。
* arguments:
    * id    = 問題番号

* return:
    読み込みに成功した場合はオブジェクトが返りますが、失敗した場合にはnull_opt()が返ります
*/
boost::optional<utils::Problem> get_problem_from_test_server(std::size_t id)
{
    const auto idstr = std::to_string(id);
    const auto url = "http://procon2014-practice.oknct-ict.org/problem/ppm/" + idstr,
               ppmFileName = "img" + idstr + ".ppm";

    if(download_ppm(url, ppmFileName))
        return utils::Problem::get(ppmFileName);
    else
        return boost::optional<utils::Problem>(boost::none);
}


/** 解答をテストサーバー(練習場)へ提出します。
*/
bool send_result_to_test_server(int id, std::string const & username, std::string const & passwd, std::string const & answer)
{
    std::vector<std::array<std::string, 2>> args(3);
    args[0][0] = "username";        args[0][1] = username;
    args[1][0] = "passwd";          args[1][1] = passwd;
    args[2][0] = "answer_text";     args[2][1] = answer;

    return curl_post("http://procon2014-practice.oknct-ict.org/solve/" + std::to_string(id), args);
}


/** 提出状況
*/
enum class SendStatus
{
    success,
    failure,
    // あと何か必要なら
};


/** 解答をサーバーへ提出します
*/
SendStatus send_result(std::string const & server_address,
                       std::string const & team_token,
                       std::string const & problem_number,
                       std::vector<std::string> const & answer)
{
    std::string ans;
    for(auto const & e: answer)
        ans += e + "\r\n";

    std::vector<std::array<std::string, 2>> args(3);
    args[0][0] = "playerid";        args[0][1] = team_token;
    args[1][0] = "problemid";       args[1][1] = problem_number;
    args[2][0] = "answer";          args[2][1] = ans;

    auto ret = curl_post("http://" + server_address + "/SubmitAnswer", args);

    if(ret)
        return SendStatus::success;
    else
        return SendStatus::failure;
}

}}   // namespace procon::inout
