#pragma once

#include <algorithm>
#include <numeric>

// http://yapb-soc.blogspot.se/2012/12/quick-and-easy-manipulating-c.html

namespace Functional
{

template <class P, class S> S filter(const P &p, S s)
{
	using F = std::function<bool(typename S::value_type)>;

	s.erase(std::remove_if(std::begin(s), std::end(s), std::not1(F(p))), std::end(s));

	return s;
}

template <class S, class _X = decltype(*std::begin(std::declval<S>())),
		  class X = typename std::decay<_X>::type>
std::vector<X> take(size_t n, const S &s)
{
	std::vector<X> r;
	std::copy_n(std::begin(s), n, std::back_inserter(r));
	return r;
}

template <class P, class S, class _X = decltype(*std::begin(std::declval<S>())),
		  class X = typename std::decay<_X>::type>
std::vector<X> takeWhile(const P &p, const S &s)
{
	std::vector<X> r;
	std::copy(std::begin(s), std::find_if(std::begin(s), std::end(s), p),
			  std::back_inserter(r));
	return r;
}

template <class S> S drop(size_t n, const S &s)
{
	return S(std::next(std::begin(s), n), std::end(s));
}

// Predicate version
template <class P, class S> S dropWhile(const P &p, const S &s)
{
	return S(std::find_if_not(std::begin(s), std::end(s), p), std::end(s));
}

template <class F, class X, class S> constexpr X foldl(F &&f, X x, const S &s)
{
	return std::accumulate(std::begin(s), std::end(s), std::move(x), std::forward<F>(f));
}

// A function wrapper that flips the argument order.
template <class F> struct Flip
{
	F f = F();

	constexpr Flip(F f) : f(std::move(f))
	{
	}

	template <class X, class Y>
	constexpr auto operator()(X &&x, Y &&y) const -> typename std::result_of<F(Y, X)>::type
	{
		return f(std::forward<Y>(y), std::forward<X>(x));
	}
};

template <class F, class X, class S> constexpr X foldr(F &&f, X x, const S &s)
{
	using It = decltype(std::begin(s));
	using RIt = std::reverse_iterator<It>;
	return std::accumulate(
		// Just foldl in reverse.
		RIt(std::end(s)), RIt(std::begin(s)), std::move(x), Flip<F>(std::forward<F>(f)));
}

template <class F, template <class...> class S, class X, class Y,
		  class Res = typename std::result_of<F(X, Y)>::type>
S<Res> zip(F &&f, const S<X> &v, const S<Y> &w)
{
	S<Res> r;
	std::transform(std::begin(v), std::end(v), std::begin(w), std::back_inserter(r),
				   std::forward<F>(f));
	return r;
}

template <class F, template <class...> class S, class X,
		  class Res = typename std::result_of<F(X)>::type>
S<Res> map(F &&f, const S<X> &s)
{
	S<Res> r;
	std::transform(std::begin(s), std::end(s), std::back_inserter(r), std::forward<F>(f));
	return r;
}

template <class F, template <class...> class S, class X, class Y,
		  class Res = typename std::result_of<F(X, Y)>::type>
S<Res> map(const F &f, const S<X> &xs, const S<Y> &ys)
{
	S<Res> r;

	for (const X &x : xs)
		for (const Y &y : ys)
			r.emplace_back(f(x, y));

	return r;
}

template <class Fold, class Map, class X, class S>
X foldMap(const Fold &f, const Map &m, X x, const S &s)
{
	for (auto &y : s)
		x = f(std::move(x), m(y));
	return x;
}

}
