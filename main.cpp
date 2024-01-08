#include <interruptible_thread.hpp>

int main()
{
    auto worker = std::make_unique<RegularInterruptibleThread>();
    auto id = worker->getId();

    auto &inttMgr = InterruptibleThreadManager::getInstance();

    inttMgr.registerThread(id, std::move(worker));
    worker = nullptr;
    inttMgr.startThread(id);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    inttMgr.interruptThread(id);
    inttMgr.joinThread(id);
    inttMgr.unregisterThread(id);

    return 0;
}