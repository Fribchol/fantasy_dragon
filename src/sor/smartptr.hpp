#pragma once

#include <memory>

namespace JanSordid::Core
{
	template <typename T, typename TDelete = std::default_delete<T>>
	class AutocastUnique : public std::unique_ptr<T, TDelete>
	{
		using Base = std::unique_ptr<T, TDelete>;

	public:
		using Base::Base;

		template <typename Y>
		constexpr AutocastUnique( Y * p ) noexcept : Base( p, TDelete() ) { static_assert( sizeof( AutocastUnique ) == sizeof( Base ) ); }

		constexpr operator       T * ()       noexcept { return this->get(); }
		constexpr operator const T * () const noexcept { return this->get(); }

		// For inheritance of contained types
	//	template<typename U> constexpr operator       U * ()       noexcept { static_assert(std::is_base_of_v<U, T> || std::is_base_of_v<T, U>); return this->get(); }
	//	template<typename U> constexpr operator const U * () const noexcept { static_assert(std::is_base_of_v<U, T> || std::is_base_of_v<T, U>); return this->get(); }
	};

	template <typename T>
	class AutocastShared : public std::shared_ptr<T>
	{
		using Base    = std::shared_ptr<T>;
		using TDelete = std::default_delete<T>;

	public:
		using Base::Base;

		template <typename Y>
		constexpr AutocastShared( Y    *  p ) noexcept : Base( p, TDelete() ) {}
		constexpr AutocastShared( Base && p ) noexcept : Base( move( p )    ) {}

		constexpr operator       T * ()       noexcept { return this->get(); }
		constexpr operator const T * () const noexcept { return this->get(); }
	};

	template <typename T>
	class AutocastWeakShare : public std::weak_ptr<T>
	{
		using Base   = std::weak_ptr<T>;
		using Shared = AutocastShared<T>;

	public:
		using Base::Base;

		Shared lock() const noexcept { return static_cast<Shared>( Base::lock() ); }
	};
}
