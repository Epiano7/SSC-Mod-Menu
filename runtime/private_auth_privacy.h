#pragma once
#include <atomic>
namespace ssc_auth_privacy {
// Stack minidumps can retain cryptographic material even after C++ objects die.
// Keep ordinary text crash diagnostics, but never write a mod-owned memory dump
// after an authentication exchange has begun in this process.
inline std::atomic<bool> secrets_used{false};
}
