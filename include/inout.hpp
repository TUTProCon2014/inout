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
#include "constants.hpp"

#define TEMPLATE_CONSTRAINTS(b) typename std::enable_if<(b)>::type *& = enabler_ptr

namespace inout{
extern void* enabler_ptr;

// 型Tが、Image型かどうかを判定します
// SFINAE
template<typename T>
constexpr auto is_image_impl(T* p)
    -> decltype((p->height(), p->width(), p->get_pixel(0u, 0u), true))
{
    return true;
}


// ditto
// SFINAE
constexpr bool is_image_impl(...)
{
    return false;
}


/// 型Tが、Image型かどうかを判定します
template <typename T>
constexpr bool is_image()
{
    return is_image_impl(static_cast<typename std::remove_reference<T>::type*>(nullptr));
}


//// 関数の引数でImageType型を取得したい場合はこのようにする
//template <typename ImageType, TEMPLATE_CONSTRAINTS(is_image<ImageType>())>
//void foo(ImageType const & image)
//{
//    ....
//}


//// オーバーロードに失敗させるときのエラーメッセージ表示用
//template <typename NotImageType, typename std::enable_if<!is_image<T>((T*)nullptr)>::type *& = enabler>
//void foo(NotImageType const & image)
//{
//    static_assert(false, "'foo' needs one image type argument.");
//}



/** pixelを表す型
*/
class Pixel
{
  public:
    Pixel(cv::Vec3b const & v) : _v(v) {}

    uint8_t b() const { return _v[0]; }
    uint8_t g() const { return _v[1]; }
    uint8_t r() const { return _v[2]; }

    cv::Vec3b vec() const { return _v; }

  private:
    cv::Vec3b _v;
};


// 
class ProblemImpl
{
  public:
    ProblemImpl(cv::Mat img) : _img(img) {}

    std::size_t height() const { return _img.rows; }
    std::size_t width() const { return _img.cols; }
    Pixel get_pixel(size_t y, size_t x) const { return Pixel(_img.at<cv::Vec3b>(y, x)); }

    cv::Mat & cvMat() { return _img; }
    cv::Mat const & cvMat() const { return _img; }

  private:
    cv::Mat _img;
};



/**
全体画像の分割画像を表すクラスです。
*/
class ElementImage
{
  public:
    ElementImage(std::shared_ptr<ProblemImpl> const & m, std::size_t r, std::size_t c, std::size_t div_x, std::size_t div_y)
    : _master(m), _pos_x(c), _pos_y(r), _div_x(div_x), _div_y(div_y)
    {}


    /// 分割画像の高さを返します
    std::size_t height() const
    {
        return _master->height() / _div_y;
    }


    /// 分割画像の幅を返します
    std::size_t width() const
    {
        return _master->width() / _div_x;
    }


    /// 分割画像の(i, j)に位置するピクセル値を返します
    Pixel get_pixel(std::size_t y, std::size_t x) const
    {
        return _master->get_pixel(y + _pos_y * height(), x + _pos_x * width());
    }


  private:
    const std::shared_ptr<ProblemImpl> _master;
    std::size_t _pos_x; // (N, M)に画像が分割されていたとき、(i, j)位置の画像を示すなら i
    std::size_t _pos_y; // (N, M)に画像が分割されていたとき、(i, j)位置の画像を示すなら j]
    std::size_t _div_x;
    std::size_t _div_y;
};


/** 問題の各種定数と画像を管理する型です。
*/
class Problem
{
  public:
    /**
    */
    static
    bool download_ppm(std::string const & url, std::string const & save_file)
    {
        auto curl_command = "curl -s -L -o " + save_file + " " + url;
        // curl
        if(std::system(curl_command.c_str()) == 0)
            return true;
        else
            return false;
    }


    /** ローカルに保存してあるppmファイルを読み込みます。
    * arguments:
        * ppm_file_path     = ローカルに保存したppmファイルへのパス
    
    * return:
        読み込みに成功した場合はオブジェクトが返りますが、失敗した場合にはnull_opt()が返ります。
    */
    static
    boost::optional<Problem> get(std::string const & ppm_file_path)
    {
        auto null_opt = [&](){ return boost::optional<Problem>(boost::none); };

        cv::Mat img = cv::imread(ppm_file_path);
        if(img.empty()){
            std::cout << "can't open " << ppm_file_path << std::endl;
            return null_opt();
        }

        Problem pb;
        pb._master = std::make_shared<ProblemImpl>(img);

        std::ifstream file(ppm_file_path);
        if(file.fail()){
            std::cout << "can't open " << ppm_file_path << std::endl;
            return null_opt();
        }

        std::string line;
        getline(file, line);    // P6
        
        char c;

        getline(file, line);    // 分割数
        std::stringstream ss1(line);
        ss1 >> c >> pb._div_x >> pb._div_y;
        
        getline(file, line);    // 最大選択可能回数
        std::stringstream ss2(line);
        ss2 >> c >> pb._max_select_times;
        
        getline(file, line);    // コスト変換レート
        std::stringstream ss3(line);
        ss3 >> c >> pb._select_cost >> pb._change_cost;

        return boost::optional<Problem>(pb);
    }


    /** サーバーから送られてきた.ppmファイルの情報からオブジェクトを構築します
    * arguments:
        * problem_data    = サーバーから送られてきた.ppmファイルへのパス


    * return:
        読み込みに成功した場合はオブジェクトが返りますが、失敗した場合にはnull_opt()が返ります。
    */
    static
    boost::optional<Problem> get(std::string const & server_address, std::string const & problem_number)
    {
        const auto url = "http://" + server_address + "/problem/prob" + problem_number + ".ppm",
                   ppmFileName = "img" + problem_number +  ".ppm";

        if(download_ppm(url, ppmFileName))
            return get(ppmFileName);
        else
            return boost::optional<Problem>(boost::none);
    }


    /** テストサーバー(練習場)から問題番号idな画像を落とし、Problemオブジェクトを構築します。
    * arguments:
        * id    = 問題番号

    * return:
        読み込みに成功した場合はオブジェクトが返りますが、失敗した場合にはnull_opt()が返ります
    */
    static
    boost::optional<Problem> get_from_test_server(uint id)
    {
        const auto idstr = std::to_string(id);
        const auto url = "http://procon2014-practice.oknct-ict.org/problem/ppm/" + idstr,
                   ppmFileName = "img" + idstr + ".ppm";

        if(download_ppm(url, ppmFileName))
            return get(ppmFileName);
        else
            return boost::optional<Problem>(boost::none);
    }


    /// 問題画像の高さを返します
    std::size_t height() const { return _master->height(); }


    /// 問題画像の幅を返します
    std::size_t width() const { return _master->width(); }


    /// (w, h) = (i, j) ピクセル目のピクセル値を返します
    Pixel get_pixel(std::size_t y, std::size_t x) const { return _master->get_pixel(y, x); }


    /// 横方向の画像分割数を返します
    std::size_t div_x() const { return _div_x; }
    

    /// 縦方向の画像分割数を返します
    std::size_t div_y() const { return _div_y; }


    /// インデックス配列におけるi行j列の断片を表すオブジェクトを返します
    ElementImage get_element(std::size_t r, std::size_t c) const
    {
        ElementImage dst(_master, r, c, _div_x, _div_y);
        return dst;
    }


    /// 交換レートを返します
    int change_cost() const { return _change_cost; }
    

    /// 選択レートを返します
    int select_cost() const { return _select_cost; }


    /// 最大選択可能回数
    std::size_t max_select_times() const { return _max_select_times; }

    cv::Mat & cvMat() { return _master->cvMat(); }
    cv::Mat const & cvMat() const { return _master->cvMat(); }


  private:
    std::shared_ptr<ProblemImpl> _master;
    std::size_t _div_x;
    std::size_t _div_y;
    int _change_cost;
    int _select_cost;
    std::size_t _max_select_times;
}; 


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
            escaped << '%' << std::setw(2) << (int)c << std::setw(0);
    }

    return escaped.str();
}


bool curl_post(std::string const & url, std::vector<std::array<std::string, 2>> const & args)
{
    std::string command = "curl -s -X POST";
    for(auto& s : args)
        command += " -d " + s[0] + "=" + url_encode(s[1]);

    // std::cout << "post: " << command + " " + url << std::endl;
    if(system((command + " " + url + (utils::buildTarget == utils::Target::Win32 ? " > nul" : " > /dev/null")).c_str()) == 0)
        return true;
    else
        return false;
}


/** 解答をテストサーバー(練習場)へ提出します。
*/
bool send_result_to_test_server(int id, std::string const & username, std::string const & passwd, std::string const & answer)
{
    std::vector<std::array<std::string, 2>> args;
    args.push_back({"username", username});
    args.push_back({"passwd", passwd});
    args.push_back({"answer_text", answer});

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

    std::vector<std::array<std::string, 2>> args;
    args.push_back({"playerid", team_token});
    args.push_back({"problemid", problem_number});
    args.push_back({"answer", ans});

    auto ret = curl_post(server_address, args);

    if(ret)
        return SendStatus::success;
    else
        return SendStatus::failure;
}

}   // namespace inout{
