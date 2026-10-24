#include "SceTypes.hpp"
#include "prx/libkernel/Pthread/include/Atfork.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI scePthreadAttrInit(PthreadAttr* attr);
int APS5_VABI scePthreadAttrDestroy(PthreadAttr* attr);
int APS5_VABI scePthreadAttrGetscope(const PthreadAttr* attr, int* scope);
int APS5_VABI scePthreadAttrSetscope(PthreadAttr* attr, int scope);
int APS5_VABI scePthreadAtfork(PthreadAtforkHandler prepare, PthreadAtforkHandler parent, PthreadAtforkHandler child);
int APS5_VABI scePthreadSetconcurrency(int level);
int APS5_VABI scePthreadGetconcurrency();
int APS5_VABI cpuset_getid_nid_postfix(int level, int which, std::int64_t id, int* setid);
int APS5_VABI cpuset_setid_nid_postfix(int which, std::int64_t id, int setid);
int APS5_VABI scePthreadGetthreadid();
int APS5_VABI getpid_nid_postfix();
int* APS5_VABI __error_nid_postfix();
}

static constexpr int SCE_KERNEL_ERROR_EINVAL = static_cast<int>(0x80020016);
static constexpr int SCOPE_PROCESS = 0;
static constexpr int SCOPE_SYSTEM = 2;

static void Require(bool value) { if (!value) std::abort(); }

static void APS5_VABI Prepare() {}
static void APS5_VABI Parent() {}
static void APS5_VABI Child() {}

static void TestScope() {
    PthreadAttr attr{};
    Require(scePthreadAttrInit(&attr) == 0);
    int scope = -1;
    Require(scePthreadAttrGetscope(&attr, &scope) == 0 && scope == SCOPE_SYSTEM);
    Require(scePthreadAttrSetscope(&attr, SCOPE_PROCESS) == 0);
    Require(scePthreadAttrGetscope(&attr, &scope) == 0 && scope == SCOPE_PROCESS);
    for (const int invalid : {1, 3, -1}) {
        Require(scePthreadAttrSetscope(&attr, invalid) == SCE_KERNEL_ERROR_EINVAL);
        scope = -1;
        Require(scePthreadAttrGetscope(&attr, &scope) == 0 && scope == SCOPE_PROCESS);
    }
    Require(scePthreadAttrSetscope(&attr, SCOPE_SYSTEM) == 0);
    Require(scePthreadAttrGetscope(&attr, &scope) == 0 && scope == SCOPE_SYSTEM);
    Require(scePthreadAttrGetscope(&attr, nullptr) == SCE_KERNEL_ERROR_EINVAL);
    Require(scePthreadAttrGetscope(nullptr, &scope) == SCE_KERNEL_ERROR_EINVAL);
    Require(scePthreadAttrSetscope(nullptr, SCOPE_PROCESS) == SCE_KERNEL_ERROR_EINVAL);
    Require(scePthreadAttrDestroy(&attr) == 0);
}

static void TestConcurrency() {
    Require(scePthreadSetconcurrency(3) == 0 && scePthreadGetconcurrency() == 3);
    Require(scePthreadSetconcurrency(-1) == SCE_KERNEL_ERROR_EINVAL && scePthreadGetconcurrency() == 3);
    Require(scePthreadSetconcurrency(0) == 0 && scePthreadGetconcurrency() == 0);
}

static void TestAtfork() {
    const auto before = PthreadAtforkHandlers_nid_no_patch().size();
    Require(scePthreadAtfork(Prepare, Parent, Child) == 0);
    Require(scePthreadAtfork(nullptr, nullptr, nullptr) == 0);
    const auto handlers = PthreadAtforkHandlers_nid_no_patch();
    Require(handlers.size() == before + 2);
    Require(handlers[before].prepare == Prepare && handlers[before].parent == Parent && handlers[before].child == Child);
    Require(handlers[before + 1].prepare == nullptr && handlers[before + 1].parent == nullptr && handlers[before + 1].child == nullptr);
}

static void TestCpusetId() {
    int setid = -1;
    Require(cpuset_getid_nid_postfix(3, 1, -1, &setid) == 0 && setid == 1);
    setid = -1;
    Require(cpuset_getid_nid_postfix(3, 1, scePthreadGetthreadid(), &setid) == 0 && setid == 1);
    setid = -1;
    Require(cpuset_getid_nid_postfix(3, 2, -1, &setid) == 0 && setid == 1);
    setid = -1;
    Require(cpuset_getid_nid_postfix(3, 2, 0, &setid) == 0 && setid == 1);
    setid = -1;
    Require(cpuset_getid_nid_postfix(3, 2, getpid_nid_postfix(), &setid) == 0 && setid == 1);

    setid = -1;
    Require(cpuset_getid_nid_postfix(1, 1, -1, &setid) == 0 && setid == 1);
    setid = -1;
    Require(cpuset_getid_nid_postfix(2, 1, -1, &setid) == 0 && setid == 1);

    Require(cpuset_getid_nid_postfix(3, 1, -1, nullptr) == -1 && *__error_nid_postfix() == 14);

    for (const int level : {0, 4, -1}) {
        Require(cpuset_getid_nid_postfix(level, 1, -1, &setid) == -1 && *__error_nid_postfix() == 22);
    }

    for (const int which : {0, 7, -1}) {
        Require(cpuset_getid_nid_postfix(3, which, -1, &setid) == -1 && *__error_nid_postfix() == 22);
    }

    Require(cpuset_getid_nid_postfix(3, 2, getpid_nid_postfix() + 1000, &setid) == -1 && *__error_nid_postfix() == 3);
    Require(cpuset_getid_nid_postfix(3, 1, 9999999, &setid) == -1 && *__error_nid_postfix() == 3);

    Require(cpuset_setid_nid_postfix(1, -1, 1) == 0);
    Require(cpuset_setid_nid_postfix(1, scePthreadGetthreadid(), 1) == 0);
    Require(cpuset_setid_nid_postfix(2, -1, 1) == 0);
    Require(cpuset_setid_nid_postfix(2, 0, 1) == 0);
    Require(cpuset_setid_nid_postfix(2, getpid_nid_postfix(), 1) == 0);
    Require(cpuset_setid_nid_postfix(1, -1, -1) == 0);

    Require(cpuset_setid_nid_postfix(1, -1, 2) == -1 && *__error_nid_postfix() == 3);
    Require(cpuset_setid_nid_postfix(2, -1, 0) == -1 && *__error_nid_postfix() == 3);

    for (const int which : {0, 7, -1}) {
        Require(cpuset_setid_nid_postfix(which, -1, 1) == -1 && *__error_nid_postfix() == 22);
    }
    Require(cpuset_setid_nid_postfix(2, getpid_nid_postfix() + 1000, 1) == -1 && *__error_nid_postfix() == 3);
    Require(cpuset_setid_nid_postfix(1, 9999999, 1) == -1 && *__error_nid_postfix() == 3);

    for (int which = 3; which <= 6; ++which) {
        bool getThrew = false;
        try { cpuset_getid_nid_postfix(3, which, -1, &setid); } catch (const std::runtime_error&) { getThrew = true; }
        Require(getThrew);

        bool setThrew = false;
        try { cpuset_setid_nid_postfix(which, -1, 1); } catch (const std::runtime_error&) { setThrew = true; }
        Require(setThrew);
    }
}

int main() {
    TestScope();
    TestConcurrency();
    TestAtfork();
    TestCpusetId();
    return 0;
}
