// Intentionally not using pch...
#include "catch.hpp"

// Only need winrt/Windows.Foundation.Collections.h for IIterable support
#include "winrt/Windows.Foundation.Collections.h"

using namespace winrt;
using namespace Windows::Foundation::Collections;

namespace
{
    IIterable<hstring> Generator()
    {
        co_yield L"Hello";
        co_yield L"World!";
    }

    class set_true_on_destruction
    {
    public:
        set_true_on_destruction(bool& value) noexcept : m_value{ &value }
        {
        }

        set_true_on_destruction(set_true_on_destruction const&) = delete;
        set_true_on_destruction& operator=(set_true_on_destruction const&) = delete;

        set_true_on_destruction(set_true_on_destruction&& other) noexcept : m_value{ std::exchange(other.m_value, nullptr) }
        {
        }

        set_true_on_destruction& operator=(set_true_on_destruction&& other) noexcept
        {
            m_value = std::exchange(other.m_value, nullptr);
            return *this;
        }

        ~set_true_on_destruction()
        {
            if (m_value)
            {
                *m_value = true;
            }
        }

    private:
        bool* m_value{nullptr};
    };

    IIterable<hstring> GeneratorSetTrueOnDestruction(set_true_on_destruction)
    {
        co_yield L"Hello";
        co_yield L"World!";
    }

    IIterable<hstring> GeneratorSetTrueOnDestructionWithException(set_true_on_destruction)
    {
        co_yield L"Hello";

        throw hresult_invalid_argument{};

        co_yield L"World!";
    }
}

TEST_CASE("coro_collections")
{
    auto generator = Generator();
    auto iterator = generator.First();
    REQUIRE(iterator.HasCurrent());
    REQUIRE(iterator.Current() == L"Hello");

    REQUIRE(iterator.MoveNext());
    REQUIRE(iterator.HasCurrent());
    REQUIRE(iterator.Current() == L"World!");

    REQUIRE(!iterator.MoveNext());
    REQUIRE(!iterator.HasCurrent());
}

TEST_CASE("coro_collections_first")
{
    auto generator = Generator();
    auto iterator = generator.First();

    REQUIRE_THROWS_AS(generator.First(), hresult_changed_state);
}

TEST_CASE("coro_collections_get_many")
{
    {
        auto iterator = Generator().First();

        std::array<hstring, 2> values;
        REQUIRE(iterator.GetMany(values) == 2);
        REQUIRE(values[0] == L"Hello");
        REQUIRE(values[1] == L"World!");
        REQUIRE(iterator.GetMany(values) == 0);
    }

    {
        auto iterator = Generator().First();
        std::array<hstring, 1> values;
        REQUIRE(iterator.GetMany(values) == 1);
        REQUIRE(values[0] == L"Hello");
        REQUIRE(iterator.HasCurrent() && iterator.Current() == L"World!");

        REQUIRE(iterator.GetMany(values) == 1);
        REQUIRE(values[0] == L"World!");
        REQUIRE(iterator.GetMany(values) == 0);
    }

}

TEST_CASE("coro_collections_coroutine_destruction")
{
    bool destroyed = false;
    {
        auto a = GeneratorSetTrueOnDestruction(destroyed);
    }

    REQUIRE(destroyed);

    destroyed = false;

    {
        IIterator<hstring> iterator;
        {
            auto generator = GeneratorSetTrueOnDestruction(destroyed);
            iterator = generator.First();
        }

        REQUIRE(!destroyed);
        REQUIRE(iterator.HasCurrent() && iterator.Current() == L"Hello");
    }

    REQUIRE(destroyed);
}

TEST_CASE("coro_collections_coroutine_destruction_with_exception")
{
    bool destroyed = false;
    auto iterator = GeneratorSetTrueOnDestructionWithException(destroyed).First();
    REQUIRE_THROWS_AS(iterator.MoveNext(), hresult_invalid_argument);
}

TEST_CASE("coro_collections_stl_for")
{
    std::wstring result;
    for (const auto &i : Generator())
    {
        result += i;
    }

    REQUIRE(result == L"HelloWorld!");
}
