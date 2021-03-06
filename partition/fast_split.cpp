//
// Created by weining on 15/12/20.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <algorithm>
#include <string_view>
#include <vector>
#include <iostream>
#include <iterator>
#include <optional>
#include <execution>
#include <cassert>

#include <autotimer.hh>

// range-v3 and c++20 ranges api both offer a cleaner (but not necessarily faster) way of splitting
// string, i.e. tokenization.
// it is based on the range view and its split() algorithm;

// see cxxFP/data_structure/ranges
//

// c++ 17 in detail P/151
// NOTE: see P/171 on fast std::search() - I can use boyer_moore searcher,
// instead of std::find_first_of() to make it even faster

std::vector< std::string_view > splitSV( std::string_view strv, std::string_view delim )
{
    std::vector< std::string_view > oxs;
    auto first = strv.begin();
    auto begin = strv.begin();
    while ( first != strv.end() )
    {
        const auto second = std::search(
            first,
            std::cend( strv ),
            std::boyer_moore_horspool_searcher( std::cbegin( delim ), std::cend( delim ) ) );
        if ( first != second )
        {
            oxs.emplace_back(
                strv.substr( std::distance( begin, first ), std::distance( first, second ) ) );
        }
        if ( second == strv.end() )
        {
            break;
        }
        first = std::next( second );
    }
    return oxs;
}

TEST_CASE( "split to vector" )
{
    std::string s( 1000000, '\0' );
    std::generate( s.begin(), s.end(), [ idx = 0 ]() mutable {
        ++idx;
        if ( ( 300000 < idx && idx < 300010 ) || ( 600000 < idx && idx < 600010 ) )
        {
            return '.';
        }
        else
        {
            return 'x';
        }
    } );
    auto oxs = splitSV( s, "........." );
    CHECK_EQ( oxs.size(), 3 );

    for ( auto sv : oxs )
    {
        auto beginOffset = sv.data() - s.data();
        std::cout << '(' << beginOffset << ", " << beginOffset + sv.size() << ", " << sv.size()
                  << ")\n";
    }
}

// making it a forward iterator
// the difficult part is to properly handle the completion and post-completion status

class SplitIterator : std::iterator< SplitIterator, std::string_view >
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::string_view;
    using difference_type = std::size_t;
    using pointer = std::string_view*;
    using reference = std::string_view&;

    SplitIterator() noexcept = default;
    SplitIterator( std::string_view haystack_, std::string_view delim_ )
        : haystack( haystack_ )
        , delim( delim_ )
        , first( haystack_.begin() )
        , begin( haystack_.begin() )
    {
        ++( *this );
    }

    bool operator==( const SplitIterator& other )
    {
        return haystack == other.haystack && delim == other.delim && first == other.first
               && begin == other.begin && second == other.second;
    }

    bool operator!=( const SplitIterator& other )
    {
        return !operator==( other );
    }

    std::string_view operator*() const
    {
        return curr;
    }

    SplitIterator& operator++()
    {
        if ( haystack.empty() || delim.empty() )
        {
            // post completion
            return *this;
        }
        if ( first == haystack.end() || second == haystack.end() )
        {
            // completion; when there is no trailing delim pattern, check for second ==
            // haystack.end(); otherwise I should check for first == haystack.end() (the second
            // pointer is not moved)
            haystack = {};
            delim = {};
            begin = {};
            first = {};
            second = {};
            return *this;
        }
        while ( first != haystack.end() )
        {
            // in-progress
            second = std::search(
                first,
                std::cend( haystack ),
                std::boyer_moore_horspool_searcher( std::cbegin( delim ), std::cend( delim ) ) );
            if ( first == second )
            {
                // if there are multiple delimiter patterns, this will skip all of them
                curr = {};
                first = std::next( second );
                continue;
            }
            // populate the curr member to point to the current token;
            // it could be the end of tokenization, hence the next call to ++() will fall into the
            // body of if-statement to mark the all positions invalid
            curr = haystack.substr( std::distance( begin, first ), std::distance( first, second ) );
            first = std::next( second );
            return *this;
        }

        // if I get here, it means it reaches completion
        haystack = {};
        delim = {};
        begin = {};
        first = {};
        second = {};
        return *this;
    }

private:
    std::string_view haystack;
    std::string_view delim;
    std::string_view curr{};

    std::string_view::const_iterator first{};
    std::string_view::const_iterator begin{};
    std::string_view::const_iterator second{};
};

TEST_CASE( "split iterator" )
{
    // with trailing delim
    {
        std::string haystack{ "there is  a cow  133  " };
        auto xs = splitSV( haystack, " " );
        CHECK_EQ( xs.size(), 5 );

        std::vector< std::string_view > ys;
        std::copy( SplitIterator{ haystack, " " }, SplitIterator{}, std::back_inserter( ys ) );
        CHECK_EQ( xs, ys );
    }
    // without trailing delim
    {
        std::string haystack{ "there is  a cow  133" };
        auto xs = splitSV( haystack, " " );
        CHECK_EQ( xs.size(), 5 );

        std::vector< std::string_view > ys;
        std::copy( SplitIterator{ haystack, " " }, SplitIterator{}, std::back_inserter( ys ) );
        CHECK_EQ( xs, ys );
    }
}

TEST_CASE( "split to vector and split iterator perf test" )
{
    // when the number of words is small (<1000), using vector of string_view is almost as fast as
    // using the iterator;
    // the vector has the advantage of SIMD (via unseq) but iterator does not as it is a forward
    // iterator.

    // see the next example that uses a large string object (100k characters)
    // the iterator version is slightly faster than the vector version (almost negligible)

    std::string text{ R"(there is a cow  there is a cow   there is a cow
there is a cow  there is a cow   there is a cow
there is a cow  there is a cow   there is a cow
there is a cow  there is a cow   there is a cow
there is a cow  there is a cow   there is a cow
there is a cow  there is a cow   there is a cow
there is a cow  there is a cow   there is a cow
there is a cow  there is a cow   there is a cow
)" };
    AutoTimer::Builder()
        .withLabel( "compare split to vector and split iterator" )
        .withMultiplier( 20 )
        .measure(
            //
            "split to vector",
            //
            [ &text ]() {
                auto xs = splitSV( text, " " );
                assert( xs.size() == 89 );
            } )
        .measure(
            //
            "split iterator",
            //
            [ &text ]() {
                auto sz = std::distance( SplitIterator{ text, " " }, SplitIterator{} );
                assert( sz == 89 );
            } );
}

// see cxxString/aggregate/repeat.cpp
std::string repeat( std::string_view pattern, size_t n )
{
    std::string o( pattern.size() * n, '\0' );
    for ( ; n > 0; --n )
    {
        std::copy( std::execution::unseq,
                   std::cbegin( pattern ),
                   std::cend( pattern ),
                   std::next( std::begin( o ), std::size( pattern ) * ( n - 1 ) ) );
    }
    return o;
}

TEST_CASE( "split to vector vs split iterator performance testing with large string object" )
{
    auto text = repeat( "there is a cow  there is a cow   there is a cow ", 10000 );
    AutoTimer::Builder()
        .withLabel( "compare split to vector and split iterator using a large string object" )
        .measure(
            //
            "split to vector",
            //
            [ &text ]() {
                auto xs = splitSV( text, " " );
                assert( xs.size() == 120000 );
            } )
        .measure(
            //
            "split iterator",
            //
            [ &text ]() {
                auto sz = std::distance( SplitIterator{ text, " " }, SplitIterator{} );
                assert( sz == 120000 );
            } );
}