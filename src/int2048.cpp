// #include "int2048.h"
#include "include/int2048.h"

namespace sjtu {
    
namespace {
    
typedef unsigned int u32;
typedef unsigned long long u64;
typedef std::vector<u32> Digits;

const u32 BASE = 10000;
const u32 BASE_DIGITS = 4;

//---------------------
// 绝对值的一些简单操作
//---------------------

// 清除前导零
void clear_front_0(Digits &a) {
    while (!a.empty() && a.back() == 0) {
        a.pop_back();
    }
}

// 比较绝对值大小，1 表示前大于后，0 表示相等，-1 表示前小于后
int comp_abs(const Digits &a, const Digits &b) {
    if (a.size() != b.size()) {
        return a.size() > b.size() ? 1 : -1;
    }
    for (int i = static_cast<int>(a.size()) - 1; i >= 0; --i) {
        if(a[i] != b[i]) {
            return a[i] > b[i] ? 1 : -1;
        }
    }
    return 0;
}

// 绝对值加法，a += b
void add_to(Digits &a, const Digits &b) {
    if (a.size() < b.size()) {
        a.resize(b.size(), 0);
    }
    u32 carry = 0;
    for (size_t i = 0; i < a.size() && (i < b.size() || carry); ++i) {
        u64 sum = static_cast<u64>(a[i]) + (i < b.size() ? b[i] : 0) + carry;
        u64 cur = sum;
        carry = cur >= BASE;
        a[i] = carry ? cur - BASE : cur;
    }
    if (carry) {
        a.push_back(carry);
    }
}

// 绝对值减法，a -= b，要求 a >= b
void sub_from(Digits &a, const Digits &b) {
    int borrow = 0;
    for (size_t i = 0; i < a.size() && (i < b.size() || borrow); ++i) {
        int cur = static_cast<int>(a[i]) - borrow - (i < b.size() ? static_cast<int>(b[i]) : 0);
        borrow = (cur < 0);
        a[i] = static_cast<u32>(borrow ? cur + BASE : cur);
    }
    clear_front_0(a);
}

// 左移
Digits shift_left(const Digits &a, const size_t &positions) {
    if (a.empty() || positions == 0) {
        return a;
    }
    Digits result(positions, 0);
    result.insert(result.end(), a.begin(), a.end());
    return result;
}

// 右移
Digits shift_right(const Digits &a, const size_t &positions) {
    if (a.empty() || positions == 0) {
        return a;
    }
    if (positions >= a.size()) {
        return Digits();
    }
    return Digits(a.begin() + positions, a.end());
}

// 转换：u64 到 Digits
Digits from_u64(u64 value) {
    Digits result;
    while (value) {
        result.push_back(static_cast<u32>(value % BASE));
        value /= BASE;
    }
    return result;
}

// 转换：Digits 到 u64，假设结果不会溢出
u64 to_u64(const Digits &a) {
    u64 value = 0;
    for (size_t i = a.size(); i-- > 0; ) {
        value = value * BASE + a[i];
    }
    return value;
}

// 转换：Digits 到字符串
std::string to_string(bool negative, const Digits &a) {
    if (a.empty()) {
        return "0";
    }
    std::string result;
    result.reserve(a.size() * BASE_DIGITS + 1);
    if (negative) {
        result.push_back('-');
    }
    char num[BASE_DIGITS];
    for (size_t i = a.size(); i-- > 0; ) {
        u32 value = a[i];
        for (int j = BASE_DIGITS - 1; j >= 0; --j) {
            num[j] = static_cast<char>(value % 10 + '0');
            value /= 10;
        }
        if (i + 1 == a.size()) {
            int start = 0;
            while (start < BASE_DIGITS && num[start] == '0') {
                ++start;
            }
            result.append(num + start, BASE_DIGITS - start);
        }
        else {
            result.append(num, BASE_DIGITS);
        }
    }
    return result;
}

//--------------
// 乘法，小范围暴力乘，大范围双模 ntt
//--------------

const u32 MOD1 = 998244353;// 119 * 2^23 + 1
const u32 MOD2 = 469762049;// 7 * 2^26 + 1
const u32 ROOT = 3;// 原根
const u32 INV1 = 332748118;
const u32 INV2 = 156587350;
// 单次 ntt 最大支持的长度
const size_t MAX_NTT_LEN = static_cast<size_t>(1) << 23;
// 朴素乘法长度限制
const size_t NAIVE_LIMIT = 48;

template<u32 MOD> u32 qpow(u32 base, u32 exp) {
    u64 result = 1;
    u64 b = base;
    while (exp) {
        if (exp & 1) {
            result = result * b % MOD;
        }
        b = b * b % MOD;
        exp >>= 1;
    }
    return static_cast<u32>(result);
}

// 旋转表
template<u32 MOD, u32 root> struct pro_table {
    Digits w;
    void build(const size_t &n) {
        const size_t old_size = w.size() ? w.size() : 1;
        w.resize(n);
        for (size_t len = old_size; len < n; len <<= 1) {
            const u32 w_len = qpow<MOD>(root, (MOD - 1) / (len << 1));
            w[len] = 1;
            for (size_t i = 1; i < len; ++i) {
                w[len + i] = static_cast<u64>(w[len + i - 1]) * w_len % MOD;
            }
        }
    }
};

template<u32 MOD, u32 root> const pro_table<MOD, root> &get_pro_table(const size_t &n) {
    static pro_table<MOD, root> table;
    if(table.w.size() < n) {
        table.build(n);
    }
    return table;
}

// ntt
template<u32 MOD> void ntt(u32 *a, size_t n, bool invert, const u32 *rev) {
    for (size_t i = 0; i < n; ++i) {
        if (i < rev[i]) {
            std::swap(a[i], a[rev[i]]);
        }
    }
    const u32 *w = invert 
                   ? (MOD == MOD1 ? get_pro_table<MOD1, INV1>(n).w.data() : get_pro_table<MOD2, INV2>(n).w.data()) 
                   : get_pro_table<MOD, ROOT>(n).w.data();
    for (size_t len = 1; len < n; len <<= 1) {
        for (size_t i = 0; i < n; i += (len << 1)) {
            for (size_t j = 0; j < len; ++j) {
                u32 u = a[i + j];
                u32 v = static_cast<u64>(a[i + j + len]) * w[len + j] % MOD;
                a[i + j] = u + v < MOD ? u + v : u + v - MOD;
                a[i + j + len] = u >= v ? u - v : u + MOD - v;
            }
        }
    }
    if (invert) {
        u32 inv_n = qpow<MOD>(n, MOD - 2);
        for (size_t i = 0; i < n; ++i) {
            a[i] = static_cast<u64>(a[i]) * inv_n % MOD;
        }
    }
}


// 数组卷积
template<u32 MOD> Digits convolve(const Digits &a, const Digits &b, size_t n) {
    Digits f(n, 0);
    Digits g(n, 0);
    for (size_t i = 0; i < a.size(); ++i) {
        f[i] = a[i];
    }
    for (size_t i = 0; i < b.size(); ++i) {
        g[i] = b[i];
    }
    
    Digits rev(n, 0);
    for(size_t i = 0; i < n; ++i) {
        rev[i] = (rev[i >> 1] >> 1) | ((i & 1) ? n >> 1 : 0);
    }
    
    ntt<MOD>(f.data(), n, false, rev.data());
    ntt<MOD>(g.data(), n, false, rev.data());
    for (size_t i = 0; i < n; ++i) {
        f[i] = static_cast<u64>(f[i]) * g[i] % MOD;
    }
    ntt<MOD>(f.data(), n, true, rev.data());
    return f;
}

// 将未进位的数组转换成标准的 BASE 进制数组
Digits normalize(const std::vector<u64> &a) {
    Digits result;
    result.reserve(a.size() + 2);
    u64 carry = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        u64 cur = a[i] + carry;
        carry = cur / BASE;
        result.push_back(static_cast<u32>(cur % BASE));
    }
    while (carry) {
        result.push_back(static_cast<u32>(carry % BASE));
        carry /= BASE;
    }
    clear_front_0(result);
    return result;
}

// 朴素乘法实现
Digits mul_naive(const Digits &a, const Digits &b) {
    std::vector<u64> result(a.size() + b.size() - 1, 0);
    for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < b.size(); ++j) {
            result[i + j] += static_cast<u64>(a[i]) * b[j];
        }
    }
    return normalize(result);
}

// 双模 ntt，这里主要做 crt 合并
Digits mul_ntt(const Digits &a, const Digits &b) {
    const size_t len = a.size() + b.size() - 1;
    size_t n = 1;
    while (n < len) {
        n <<= 1;
    }
    const Digits res1 = convolve<MOD1>(a, b, n);
    const Digits res2 = convolve<MOD2>(a, b, n);
    const u64 inv_MOD1 = qpow<MOD2>(MOD1 % MOD2, MOD2 - 2);
    std::vector<u64> result(len, 0);
    for(int i = 0; i < len; ++i) {
        const u64 t = (res2[i] + MOD2 - res1[i] % MOD2) % MOD2 * inv_MOD1 % MOD2;
        result[i] = res1[i] + t * static_cast<u64>(MOD1);
    }
    return normalize(result);
}

Digits mul(const Digits &a, const Digits &b);

// 单次长度超过 MAX_NTT_LEN 时平分较长一个进行乘法
Digits mul_split(const Digits &a, const Digits &b) {
    const Digits &large = (a.size() >= b.size() ? a : b);
    const Digits &small = (a.size() >= b.size() ? b : a);
    const size_t mid = large.size() / 2;
    Digits low(large.begin(), large.begin() + mid);
    clear_front_0(low);
    const Digits high(large.begin() + mid, large.end());
    Digits result = mul(low, small);
    add_to(result, shift_left(mul(high, small), mid));
    return result;
}

// 乘法函数
Digits mul(const Digits &a, const Digits &b) {
    if (a.empty() || b.empty()) {
        return Digits();
    }
    if (std::min(a.size(), b.size()) < NAIVE_LIMIT) {
        return mul_naive(a, b);
    }
    if (a.size() + b.size() - 1 <= MAX_NTT_LEN) {
        return mul_ntt(a, b);
    }
    return mul_split(a, b);
}

//------------
// 除法，牛顿迭代 + 双模 ntt
//------------

// 估商时额外保留的位数
const size_t GUARD_BLOCKS = 3;
// 小于该长度使用朴素除法
const size_t SHORT_DIV_LIMIT = 3;

// 朴素除法
void divmod_short(const Digits &a, u64 b, Digits &q, Digits &r) {
    q.assign(a.size(), 0);
    u64 rest = 0;
    for (size_t i = a.size(); i-- > 0; ) {
        rest = rest * BASE + a[i];
        q[i] = rest / b;
        rest %= b;
    }
    clear_front_0(q);
    r = from_u64(rest);
}

// 求倒数
Digits reciprocal(const Digits &a) {
    const size_t n = a.size();
    if (n <= 2) {
        return from_u64(static_cast<u64>(BASE) * (n == 1 ? BASE : static_cast<u64>(BASE) * BASE * BASE) / to_u64(a));
    }
    
    const size_t h = n / 2 + 2 < n ? n / 2 + 2 : n - 1;
    const Digits a_h = reciprocal(shift_right(a, n - h));
    
    const Digits pro = mul(a, a_h);
    Digits unit(n + h + 1, 0);
    unit[n + h] = 1;
    const bool greater_flag = comp_abs(pro, unit) > 0;
    Digits diff = greater_flag ? pro : unit;
    sub_from(diff, greater_flag ? unit : pro);
    
    const size_t keep = n - h + 5;
    const size_t drop = diff.size() > keep ? diff.size() - keep : 0;
    Digits correction = mul(a_h, shift_right(diff, drop));
    correction = drop >= 2 * h 
                 ? shift_left(correction, drop - 2 * h) 
                 : shift_right(correction, 2 * h - drop);
    
    Digits a_ = shift_left(a_h, n - h);
    if (greater_flag) {
        sub_from(a_, correction);
    }
    else {
        add_to(a_, correction);
    }
    return a_;
}

// 修正商
void divmod_correction(const Digits &a, const Digits &b, const Digits &reciprocal_b, const size_t &s, Digits &q, Digits &r) {
    if (comp_abs(a, b) < 0) {
        q.clear();
        r = a;
        return ;
    }
    const size_t drop = a.size() > s + 2 ? a.size() - (s + 2) : 0;
    q = shift_right(mul(shift_right(a, drop), reciprocal_b), s + b.size() - drop);
    
    Digits one(1, 1);
    Digits pro = mul(b, q);
    while (comp_abs(pro, a) > 0) {
        sub_from(q, one);
        sub_from(pro, b);
    }
    r = a;
    sub_from(r, pro);
    while(comp_abs(r, b) >= 0) {
        sub_from(r, b);
        add_to(q, one);
    }
}

// 除法函数
void divmod_abs(const Digits &a, const Digits &b, Digits &q, Digits &r) {
    if (comp_abs(a, b) < 0) {
        q.clear();
        r = a;
        return ;
    }
    const size_t n = a.size(), m = b.size();
    if (m <= SHORT_DIV_LIMIT) {
        divmod_short(a, to_u64(b), q, r);
        return ;
    }
    
    const size_t k = n - m + 1;
    const size_t c = k < m ? k : m;
    const size_t s = c + GUARD_BLOCKS;
    const Digits reciprocal_b = reciprocal(s <= m ? shift_right(b, m - s) : shift_left(b, s - m));
    
    q.assign(k, 0);
    r = shift_right(a, k);
    Digits a_, q_;
    for (size_t hi = k; hi > 0;) {
        const size_t take = hi < s ? hi : s;
        a_.assign(a.begin() + hi - take, a.begin() + hi);
        a_.insert(a_.end(), r.begin(), r.end());
        clear_front_0(a_);
        divmod_correction(a_, b, reciprocal_b, s, q_, r);
        for(size_t i = 0; i < q_.size(); ++i) {
            q[hi - take + i] = q_[i];
        }
        hi -= take;
    }
    clear_front_0(q);
}

} // namespace

//---------------------
// 构造，赋值，输入输出
//---------------------

// 构造函数：默认构造，初始化为 0
int2048::int2048() : negative_(false), digits_() {}

// 构造函数：从 long long 初始化，处理符号并转换为 Digits
int2048::int2048(long long value) : negative_(value < 0) {
    const u64 abs_value = value < 0 ? 0ULL - static_cast<u64>(value) : static_cast<u64>(value);
    digits_ = from_u64(abs_value);
}

// 构造函数：从字符串初始化，解析符号并转换为 Digits
int2048::int2048(const std::string &str) : negative_(false), digits_() {
    read(str);
}

// 构造函数：拷贝构造
int2048::int2048(const int2048 &other) : negative_(other.negative_), digits_(other.digits_) {}

// 构造函数：移动构造
int2048::int2048(int2048 &&other) noexcept: negative_(other.negative_) {
    digits_.swap(other.digits_);
    other.negative_ = false;
    other.digits_.clear();
}

// 赋值运算符：拷贝赋值
int2048 &int2048::operator=(const int2048 &other) {
    if (this != &other) {
        negative_ = other.negative_;
        digits_ = other.digits_;
    }
    return *this;
}

// 赋值运算符：移动赋值
int2048 &int2048::operator=(int2048 &&other) noexcept {
    if (this != &other) {
        const bool negative = negative_;
        negative_ = other.negative_;
        other.negative_ = negative;
        digits_.swap(other.digits_);
    }
    return *this;
}


// 读取字符串，解析符号并转换为 Digits
void int2048::read(const std::string &str) {
    negative_ = false;
    size_t begin = 0;
    if (!str.empty() && (str[begin] == '-' || str[begin] == '+')) {
        negative_ = (str[begin++] == '-');
    }
    while (begin < str.size() && str[begin] == '0') {
        ++begin;
    }
    
    digits_.clear();
    digits_.reserve((str.size() - begin + BASE_DIGITS - 1) / BASE_DIGITS);
    for (size_t hi = str.size(); hi > begin; ) {
        size_t lo = (hi - begin > BASE_DIGITS ? hi - BASE_DIGITS : begin);
        u32 value = 0;
        for (size_t i = lo; i < hi; ++i) {
            value = value * 10 + static_cast<u32>(str[i] - '0');
        }
        digits_.push_back(value);
        hi = lo;
    }
    if (digits_.empty()) {
        negative_ = false;
    }
}

// 输出储存的大整数，无需换行
void int2048::print() {
    std::string str = to_string(negative_, digits_);
    std::fwrite(str.data(), 1, str.size(), stdout);
}

// 重载输入
std::istream &operator>>(std::istream &is, int2048 &value) {
    std::string str;
    if (is >> str) {
        value.read(str);
    }
    return is;
}

// 重载输出
std::ostream &operator<<(std::ostream &os, const int2048 &value) {
    return os << to_string(value.negative_, value.digits_);
}

//-----------------
// 比较
//-----------------

// 比较两个大整数的大小，返回值含义：1 表示前大于后，0 表示相等，-1 表示前小于后
int int2048::comp(const int2048 &a, const int2048 &b) {
    if (a.negative_ != b.negative_) {
        return a.negative_ ? -1 : 1;
    }
    const int cmp = comp_abs(a.digits_, b.digits_);
    return a.negative_ ? -cmp : cmp;
}

// 重载比较运算符，基于 comp 函数实现
bool operator==(const int2048 &lhs, const int2048 &rhs) {
    return int2048::comp(lhs, rhs) == 0;
}
bool operator!=(const int2048 &lhs, const int2048 &rhs) {
    return int2048::comp(lhs, rhs) != 0;
}
bool operator<(const int2048 &lhs, const int2048 &rhs) {
    return int2048::comp(lhs, rhs) < 0;
}
bool operator>(const int2048 &lhs, const int2048 &rhs) {
    return int2048::comp(lhs, rhs) > 0;
}
bool operator<=(const int2048 &lhs, const int2048 &rhs) {
    return int2048::comp(lhs, rhs) <= 0;
}
bool operator>=(const int2048 &lhs, const int2048 &rhs) {
    return int2048::comp(lhs, rhs) >= 0;
}

//--------------
// 加减法
//--------------

// 加减法函数
int2048 &int2048::signed_add(const int negative, const Digits &a) {
    if (a.empty()) {
        return *this;
    }
    
    if (negative_ == negative) {
        add_to(digits_, a);
    }
    else if (comp_abs(digits_, a) >= 0) {
        sub_from(digits_, a);
        if (digits_.empty()) {
            negative_ = false;
        }
    }
    else {
        Digits tmp = a;
        sub_from(tmp, digits_);
        digits_.swap(tmp);
        negative_ = negative;
    }
    return *this;
}

// 所有加减法都换成 += 或 -=
int2048 &int2048::operator+=(const int2048 &rhs) {
    return signed_add(rhs.negative_, rhs.digits_);
}
int2048 &int2048::operator-=(const int2048 &rhs) {
    return signed_add(!rhs.negative_, rhs.digits_);
}

int2048 &int2048::add(const int2048 &rhs) {
    return *this += rhs;
}
int2048 &int2048::minus(const int2048 &rhs) {
    return *this -= rhs;
}

int2048 add(int2048 lhs, const int2048 &rhs) {
    lhs += rhs;
    return lhs;
}
int2048 minus(int2048 lhs, const int2048 &rhs) {
    lhs -= rhs;
    return lhs;
}

int2048 operator+(int2048 lhs, const int2048 &rhs) {
    lhs += rhs;
    return lhs;
}
int2048 operator-(int2048 lhs, const int2048 &rhs) {
    lhs -= rhs;
    return lhs;
}

int2048 int2048::operator+() const {
    return *this;
}

int2048 int2048::operator-() const {
    int2048 tmp = *this;
    tmp.negative_ = !tmp.negative_ && !tmp.digits_.empty();
    return tmp;
}

//-------------
// 乘除法
//-------------

// 乘法函数
int2048 &int2048::operator*=(const int2048 &rhs) {
    const bool neagtive = negative_ ^ rhs.negative_;
    digits_ = mul(digits_, rhs.digits_);
    negative_ = neagtive && !digits_.empty();
    return *this;
}

int2048 operator*(int2048 lhs, const int2048 &rhs) {
    lhs *= rhs;
    return lhs;
}

// 除法函数
void int2048::divmod(const int2048 &lhs, const int2048 &rhs, int2048 *quotient, int2048 * remainder) {
    const bool signs_diff = lhs.negative_ ^ rhs.negative_;
    Digits q, r;
    divmod_abs(lhs.digits_, rhs.digits_, q, r);
    if (signs_diff && !r.empty()) {
        add_to(q, Digits(1, 1));
        Digits tmp = rhs.digits_;
        sub_from(tmp, r);
        r.swap(tmp);
    }
    if (quotient) {
        quotient->digits_.swap(q);
        quotient->negative_ = signs_diff && !quotient->digits_.empty();
    }
    if (remainder) {
        remainder->digits_.swap(r);
        remainder->negative_ = rhs.negative_ && !remainder->digits_.empty();
    }
}

int2048 &int2048::operator/=(const int2048 &rhs) {
    divmod(*this, rhs, this, nullptr);
    return *this;
}
int2048 &int2048::operator%=(const int2048 &rhs) {
    divmod(*this, rhs, nullptr, this);
    return *this;
}

int2048 operator/(int2048 lhs, const int2048 &rhs) {
    lhs /= rhs;
    return lhs;
}
int2048 operator%(int2048 lhs, const int2048 &rhs) {
    lhs %= rhs;
    return lhs;
}

} // namespace sjtu
