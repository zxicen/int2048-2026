#pragma once
#ifndef SJTU_BIGINTEGER
#define SJTU_BIGINTEGER

// Integer 1:
// 实现一个有符号的大整数类，只需支持简单的加减

// Integer 2:
// 实现一个有符号的大整数类，支持加减乘除，并重载相关运算符

// 请不要使用除了以下头文件之外的其它头文件
#include <complex>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

// 请不要使用 using namespace std;

namespace sjtu {
class int2048 {
    // todo
public:
    // 构造函数
    int2048();// ok
    int2048(long long);// ok
    int2048(const std::string &);// ok
    int2048(const int2048 &);// ok
    int2048(int2048 &&) noexcept;// ok

    // 以下给定函数的形式参数类型仅供参考，可自行选择使用常量引用或者不使用引用
    // 如果需要，可以自行增加其他所需的函数
    // ===================================
    // Integer1
    // ===================================

    // 读入一个大整数
    void read(const std::string &);// ok
    // 输出储存的大整数，无需换行
    void print();// ok

    // 加上一个大整数
    int2048 &add(const int2048 &);// ok
    // 返回两个大整数之和
    friend int2048 add(int2048, const int2048 &);// ok

    // 减去一个大整数
    int2048 &minus(const int2048 &);// ok
    // 返回两个大整数之差
    friend int2048 minus(int2048, const int2048 &);// ok

    // ===================================
    // Integer2
    // ===================================

    int2048 operator+() const;// ok
    int2048 operator-() const;// ok

    int2048 &operator=(const int2048 &);// ok
    int2048 &operator=(int2048 &&) noexcept;// ok

    int2048 &operator+=(const int2048 &);// ok
    friend int2048 operator+(int2048, const int2048 &);// ok

    int2048 &operator-=(const int2048 &);// ok
    friend int2048 operator-(int2048, const int2048 &);// ok

    int2048 &operator*=(const int2048 &);// ok
    friend int2048 operator*(int2048, const int2048 &);// ok

    int2048 &operator/=(const int2048 &);// ok
    friend int2048 operator/(int2048, const int2048 &);// ok

    int2048 &operator%=(const int2048 &);// ok
    friend int2048 operator%(int2048, const int2048 &);// ok

    friend std::istream &operator>>(std::istream &, int2048 &);// ok
    friend std::ostream &operator<<(std::ostream &, const int2048 &);// ok

    friend bool operator==(const int2048 &, const int2048 &);// ok
    friend bool operator!=(const int2048 &, const int2048 &);// ok
    friend bool operator<(const int2048 &, const int2048 &);// ok
    friend bool operator>(const int2048 &, const int2048 &);// ok
    friend bool operator<=(const int2048 &, const int2048 &);// ok
    friend bool operator>=(const int2048 &, const int2048 &);// ok
private:
    // 绝对值按 BASE 进制存储
    bool negative_;
    std::vector<unsigned int> digits_;
    
    // 比较两个大整数的大小，1 表示前大于后，0 表示相等，-1 表示前小于后
    static int comp(const int2048 &, const int2048 &);
    // 加减法函数
    int2048 &signed_add(const int negative, const std::vector<unsigned int> &a);
    // 除法函数，计算 lhs 除以 rhs 的商和余数，结果分别存储在 quotient 和 remainder 中
    static void divmod(const int2048 &lhs, const int2048 &rhs, int2048 *quotient, int2048 * remainder);
};
} // namespace sjtu

#endif
