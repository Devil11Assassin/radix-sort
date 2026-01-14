#pragma once
#include <algorithm>
#include <compare>
#include <concepts>
#include <limits>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace radix_sort
{
	namespace internal 
	{
		// ================
		// -----Common-----
		// ================

		namespace shared
		{
			// =======================================
			// -----Constants & Type Declarations-----
			// =======================================
			
			using Index = std::size_t; // int or std::size_t

			inline constexpr Index SHIFT_BITS = 8;
			inline constexpr Index BASE = 256;
			inline constexpr Index MASK = 0xFF;
			inline constexpr Index INVERT_MASK = 0x80;

			inline constexpr Index CHARS = 256;
			inline constexpr Index CHARS_ALLOC = 257;

			inline constexpr Index SIZE_THRESHOLD = 8'000'000;
			inline constexpr Index BUCKET_THRESHOLD = 1'000'000;
			inline constexpr Index INSERTION_SORT_THRESHOLD_STR = 10;
			inline constexpr Index INSERTION_SORT_THRESHOLD_ALL = 100;
			inline constexpr Index ALLOW_SLEEP_THRESHOLD = 1'000'000;

			struct Region
			{
				Index l;
				Index r;
				Index len;
				Index curShiftOrIndex;
				Region(Index l, Index r, Index len, Index curShiftOrIndex) : l(l), r(r), len(len), curShiftOrIndex(curShiftOrIndex) {}
			};

			// ====================================
			// -----Concepts & Type Converters-----
			// ====================================

			template <typename T>
			concept is_small_integral = std::integral<T> && sizeof(T) <= 4;

			template <typename T>
			concept is_large_integral = std::integral<T> && sizeof(T) > 4;

			template <typename T>
			concept is_floating_point = std::floating_point<T>;

			template <typename T>
			concept is_string = std::same_as<T, std::string>;

			template <typename T>
			concept known = std::integral<T> || is_floating_point<T> || is_string<T>;

			template <typename T>
			concept unknown = !known<T>;

			template <typename T, typename F>
			using invoke_result = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;

			template <typename T, typename F>
			concept sort_helper = std::invocable<F, const T&> && known<invoke_result<T, F>>;

			template <size_t S> struct t2u_impl;
			template <> struct t2u_impl<1> { using type = std::uint8_t; };
			template <> struct t2u_impl<2> { using type = std::uint16_t; };
			template <> struct t2u_impl<4> { using type = std::uint32_t; };
			template <> struct t2u_impl<8> { using type = std::uint64_t; };

			template <typename T>
			using t2u = t2u_impl<sizeof(T)>::type;

			// =================
			// -----Helpers-----
			// =================

			inline int getChar(const std::string& s, Index index)
			{
				return (index < s.length()) ? static_cast<unsigned char>(s[index]) : 256;
			}

			template <typename T>
			inline void getCountVectorThread(std::vector<T>& v, std::vector<Index>& count, Index curShiftOrIndex, Index l, Index r)
			{
				if constexpr (is_string<T>)
				{
					for (Index i = l; i < r; i++)
						count[getChar(v[i], curShiftOrIndex)]++;
				}
				else if constexpr (std::unsigned_integral<T>)
				{
					for (Index i = l; i < r; i++)
						count[(v[i] >> curShiftOrIndex) & MASK]++;
				}
				else if constexpr (std::signed_integral<T>)
				{
					constexpr Index MAX_SHIFT = (sizeof(T) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (Index i = l; i < r; i++)
							count[(static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK]++;
					}
					else
					{
						for (Index i = l; i < r; i++)
							count[((static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
					}
				}
			}

			template <typename T>
			inline void getCountVector(std::vector<T>& v, std::vector<Index>& count, Index curShiftOrIndex, Index l, Index r, bool enableMultiThreading)
			{
				const Index SIZE = r - l;
				Index threadCount = static_cast<Index>(ceil(SIZE / BUCKET_THRESHOLD));

				if (!enableMultiThreading || threadCount <= 1 || SIZE < SIZE_THRESHOLD)
				{
					getCountVectorThread(v, count, curShiftOrIndex, l, r);
				}
				else
				{
					constexpr Index ALLOC_SIZE = (is_string<T>) ? CHARS_ALLOC : BASE;

					threadCount = std::min(threadCount, static_cast<Index>(std::thread::hardware_concurrency()));
					Index bucketSize = SIZE / threadCount;

					std::vector<std::thread> threads;
					std::vector<std::vector<Index>> counts(threadCount, std::vector<Index>(ALLOC_SIZE));

					for (Index i = 0; i < threadCount; i++)
					{
						Index start = l + i * bucketSize;
						Index end = (i == threadCount - 1) ? r : start + bucketSize;
						threads.emplace_back(getCountVectorThread<T>, std::ref(v), std::ref(counts[i]), curShiftOrIndex, start, end);
					}

					for (auto& t : threads)
						t.join();

					for (Index curThread = 0; curThread < threadCount; curThread++)
					{
						for (Index i = 0; i < ALLOC_SIZE; i++)
							count[i] += counts[curThread][i];
					}
				}
			}

			template <typename T>
			inline void getPrefixVector(std::vector<Index>& prefix, std::vector<Index>& count, Index l)
			{
				if constexpr (is_string<T>)
				{
					prefix[256] = l;
					prefix[0] = prefix[256] + count[256];
				}
				else
				{
					prefix[0] = l;
				}

				for (Index i = 1; i < BASE; i++)
					prefix[i] = prefix[i - 1] + count[i - 1];
			}
		}

		// =======================
		// -----T sorted by T-----
		// =======================

		namespace value
		{
			using namespace shared;
			
			// =================
			// -----Helpers-----
			// =================
			
			template <typename T>
			inline bool isSortedBi(std::vector<T>& v)
			{
				constexpr auto cmpLess = (is_floating_point<T>) ?
					[](const T& a, const T& b) { return std::strong_order(a, b) < 0; } :
					[](const T& a, const T& b) { return a < b; };

				constexpr auto cmpGreater = (is_floating_point<T>) ?
					[](const T& a, const T& b) { return std::strong_order(a, b) > 0; } :
					[](const T& a, const T& b) { return a > b; };

				bool sortedAsc = true;
				bool sortedDesc = true;

				for (Index i = 0, size = v.size() - 1; i < size; i++)
				{
					if (sortedAsc && cmpGreater(v[i], v[i + 1]))
						sortedAsc = false;

					if (sortedDesc && cmpLess(v[i], v[i + 1]))
						sortedDesc = false;

					if (!sortedAsc && !sortedDesc)
						break;
				}

				if (sortedAsc)
					return true;
				else if (sortedDesc)
				{
					std::reverse(v.begin(), v.end());
					return true;
				}

				return false;
			}

			template <typename T>
			inline Index getMaxLength(std::vector<T>& v)
			{
				Index len = 0;

				if constexpr (std::signed_integral<T> || is_floating_point<T>)
				{
					len = sizeof(T);
				}
				else if constexpr (std::unsigned_integral<T>)
				{
					T maxVal = *std::max_element(v.begin(), v.end());

					while (maxVal > 0)
					{
						len++;
						maxVal >>= SHIFT_BITS;
					}
				}
				else if constexpr (is_string<T>)
				{
					len = (*std::max_element(v.begin(), v.end(),
						[](const std::string& a, const std::string& b) {
							return a.length() < b.length();
						})).length();
				}

				return len;
			}

			template <typename T>
			inline void insertionSort(std::vector<T>& v, Index l, Index r)
			{
				constexpr auto cmp = (is_floating_point<T>) ?
					[](const T& a, const T& b) { return std::strong_order(a, b) < 0; } :
					[](const T& a, const T& b) { return a < b; };

				for (Index i = l + 1; i < r; i++)
				{
					T val = std::move(v[i]);
					Index j = i;

					while (j > l && cmp(val, v[j - 1]))
					{
						v[j] = std::move(v[j - 1]);
						j--;
					}

					v[j] = std::move(val);
				}
			}

			template <typename T, typename U>
			inline void getConvertedVectorThread(std::vector<T>& v, std::vector<U>& vu, bool reverse, Index l, Index r)
			{
				constexpr Index SIGN_SHIFT = (sizeof(T) * 8) - 1;
				constexpr U SIGN_MASK = 1LL << SIGN_SHIFT;

				if (!reverse)
				{
					for (Index i = l; i < r; i++)
					{
						std::memcpy(&vu[i], &v[i], sizeof(T));
						vu[i] = (vu[i] >> SIGN_SHIFT) ? ~vu[i] : vu[i] ^ SIGN_MASK;
					}
				}
				else
				{
					for (Index i = l; i < r; i++)
					{
						vu[i] = (vu[i] >> SIGN_SHIFT) ? vu[i] ^ SIGN_MASK : ~vu[i];
						std::memcpy(&v[i], &vu[i], sizeof(T));
					}
				}
			}

			template <typename T, typename U>
			inline void getConvertedVector(std::vector<T>& v, std::vector<U>& vu, bool reverse, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				Index threadCount = static_cast<Index>(ceil(SIZE / BUCKET_THRESHOLD));

				if (!enableMultiThreading || threadCount <= 1 || SIZE < SIZE_THRESHOLD)
				{
					getConvertedVectorThread(v, vu, reverse, 0, SIZE);
				}
				else
				{
					std::vector<std::thread> threads;
					threadCount = std::min(threadCount, static_cast<Index>(std::thread::hardware_concurrency()));
					Index bucketSize = SIZE / threadCount;

					for (Index i = 0; i < threadCount; i++)
					{
						Index start = i * bucketSize;
						Index end = (i == threadCount - 1) ? SIZE : start + bucketSize;
						threads.emplace_back([&v, &vu, reverse, start, end]() {
							getConvertedVectorThread(v, vu, reverse, start, end);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			template <typename T>
			inline void getUpdatedVector(std::vector<T>& v, std::vector<T>& tmp, std::vector<Index>& prefix, Index curShift)
			{
				if constexpr (std::unsigned_integral<T>)
				{
					for (const auto& num : v)
						tmp[prefix[(num >> curShift) & MASK]++] = num;
				}
				else if constexpr (std::signed_integral<T>)
				{
					constexpr Index MAX_SHIFT = (sizeof(T) - 1) * 8;

					if (curShift != MAX_SHIFT)
					{
						for (const auto& num : v)
							tmp[prefix[(static_cast<t2u<T>>(num) >> curShift) & MASK]++] = num;
					}
					else
					{
						for (const auto& num : v)
							tmp[prefix[((static_cast<t2u<T>>(num) >> curShift) & MASK) ^ INVERT_MASK]++] = num;
					}
				}
			}

			template <typename T>
			inline void getUpdatedVector(std::vector<T>& v, std::vector<T>& tmp, std::vector<Index>& prefix, Index curShiftOrIndex, Index l, Index r)
			{
				if constexpr (is_string<T>)
				{
					for (Index i = l; i < r; i++)
						tmp[prefix[getChar(v[i], curShiftOrIndex)]++] = std::move(v[i]);
				}
				else if constexpr (std::unsigned_integral<T>)
				{
					for (Index i = l; i < r; i++)
						tmp[prefix[(v[i] >> curShiftOrIndex) & MASK]++] = v[i];
				}
				else if constexpr (std::signed_integral<T>)
				{
					constexpr Index MAX_SHIFT = (sizeof(T) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (Index i = l; i < r; i++)
							tmp[prefix[(static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK]++] = v[i];
					}
					else
					{
						for (Index i = l; i < r; i++)
							tmp[prefix[((static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++] = v[i];
					}
				}
			}

			template <typename T>
			inline void sortInstance(std::vector<T>& v, std::vector<T>& tmp,
				std::vector<Region>& regions, std::unique_lock<std::mutex>& lkRegions,
				Region initialRegion, bool enableMultiThreading)
			{
				std::vector<Region> regionsLocal;
				regionsLocal.reserve(v.size() / 100);
				regionsLocal.emplace_back(initialRegion);

				constexpr Index INSERTION_SORT_THRESHOLD = (is_string<T>) ? INSERTION_SORT_THRESHOLD_ALL : INSERTION_SORT_THRESHOLD_ALL;
				constexpr Index ALLOC_SIZE = (is_string<T>) ? CHARS_ALLOC : BASE;

				while (regionsLocal.size())
				{
					Region region = std::move(regionsLocal.back());
					regionsLocal.pop_back();

					Index l = region.l;
					Index r = region.r;
					Index len = region.len;
					Index curShiftOrIndex = region.curShiftOrIndex;

					if (r - l < 2 || len == 0)
						continue;

					if (r - l <= INSERTION_SORT_THRESHOLD)
					{
						insertionSort(v, l, r);
						continue;
					}

					std::vector<Index> count(ALLOC_SIZE);
					std::vector<Index> prefix(ALLOC_SIZE);

					getCountVector(v, count, curShiftOrIndex, l, r, enableMultiThreading);
					getPrefixVector<T>(prefix, count, l);
					getUpdatedVector(v, tmp, prefix, curShiftOrIndex, l, r);

					std::move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

					len--;
					if (len == 0)
						continue;

					Index start = 0;
					if constexpr (is_string<T>)
					{
						start = l + count[256];
						curShiftOrIndex++;
					}
					else
					{
						start = l;
						curShiftOrIndex -= SHIFT_BITS;
					}

					if (!enableMultiThreading)
					{
						for (Index i = 0; i < BASE; i++)
						{
							if (count[i] > 1)
								regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
							start += count[i];
						}
					}
					else
					{
						const Index LOCAL_BUCKET_THRESHOLD = std::max((r - l) / 1000, static_cast<Index>(1000));

						for (Index i = 0; i < CHARS; i++)
						{
							if (count[i] > 1)
							{
								if (count[i] < LOCAL_BUCKET_THRESHOLD)
									regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
								else
								{
									lkRegions.lock();
									regions.emplace_back(start, start + count[i], len, curShiftOrIndex);
									lkRegions.unlock();
								}
							}
							start += count[i];
						}
					}
				}
			}

			template <typename T>
			inline void sortThread(std::vector<T>& v, std::vector<T>& tmp,
				std::vector<Region>& regions, std::mutex& regionsLock,
				Index& runningCounter, Index threadIndex)
			{
				std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

				bool isIdle = false;
				Index iterationsIdle = 0;
				const bool ALLOW_SLEEP = v.size() > ALLOW_SLEEP_THRESHOLD;

				while (true)
				{
					lkRegions.lock();
					if (regions.size())
					{
						Region region = std::move(regions.back());
						regions.pop_back();

						if (isIdle)
						{
							isIdle = false;
							runningCounter++;
							iterationsIdle = 0;
						}

						lkRegions.unlock();
						sortInstance(v, tmp, regions, lkRegions, region, 1);
					}
					else
					{
						if (!isIdle)
						{
							isIdle = true;
							runningCounter--;
						}

						if (runningCounter == 0)
						{
							lkRegions.unlock();
							break;
						}

						lkRegions.unlock();
						iterationsIdle++;

						if (ALLOW_SLEEP && iterationsIdle > 100)
						{
							iterationsIdle = 0;
							std::this_thread::sleep_for(std::chrono::nanoseconds(1));
						}
					}
				}
			}
		
			// =========================
			// -----Implementations-----
			// =========================
		
			template <is_small_integral T>
			inline void sort_impl(std::vector<T>& v, Index len, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				Index curShift = 0;

				while (len--)
				{
					std::vector<Index> count(BASE);
					std::vector<Index> prefix(BASE);

					getCountVector(v, count, curShift, 0, SIZE, enableMultiThreading);
					getPrefixVector<T>(prefix, count, 0);
					getUpdatedVector(v, tmp, prefix, curShift);

					std::swap(v, tmp);

					curShift += SHIFT_BITS;
				}
			}

			template <typename T> requires (is_large_integral<T> || is_string<T>)
			inline void sort_impl(std::vector<T>& v, Index len, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				constexpr Index curShiftOrIndex = (is_string<T>) ? 0 : (sizeof(T) - 1) * 8;
				const Index LOCAL_BUCKET_THRESHOLD = std::max(static_cast<Index>(SIZE / 1000), static_cast<Index>(1000));

				if (!enableMultiThreading || static_cast<Index>(SIZE / 256) - (LOCAL_BUCKET_THRESHOLD / 10) < LOCAL_BUCKET_THRESHOLD)
				{
					std::mutex tmpMutex;
					std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

					std::vector<Region> tmpVector;
					sortInstance(v, tmp, tmpVector, tmpLock, Region(0, SIZE, len, curShiftOrIndex), false);
				}
				else
				{
					Index numOfThreads = std::thread::hardware_concurrency();
					std::vector<std::thread> threads;
					std::mutex regionsLock;
					std::mutex idleLock;
					Index runningCounter = numOfThreads;

					std::vector<Region> regions;
					regions.reserve(SIZE / 1000);
					regions.emplace_back(0, SIZE, len, curShiftOrIndex);
					for (Index i = 0; i < numOfThreads; i++)
					{
						threads.emplace_back([&v, &tmp, &regions, &regionsLock, &runningCounter, i]() {
							sortThread(v, tmp, regions, regionsLock, runningCounter, i);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			template <is_floating_point T>
			inline void sort_impl(std::vector<T>& v, Index len, bool enableMultiThreading)
			{
				using U = t2u<T>;
				const Index SIZE = v.size();
				std::vector<U> vu(SIZE);

				getConvertedVector(v, vu, false, enableMultiThreading);
				sort_impl<U>(vu, len, enableMultiThreading);
				getConvertedVector(v, vu, true, enableMultiThreading);
			}

			// =====================
			// -----Entry Point-----
			// =====================

			template <known T>
			inline void sort_dispatcher(std::vector<T>& v, bool enableMultiThreading)
			{
				if constexpr (is_floating_point<T>)
				{
					static_assert(
						std::numeric_limits<T>::is_iec559,
						"ERROR: generators.hpp requires IEEE-754/IEC-559 compliance.\n"
					);
				}

				if (isSortedBi(v))
					return;

				constexpr Index INSERTION_SORT_THRESHOLD = (is_string<T>) ? INSERTION_SORT_THRESHOLD_STR : INSERTION_SORT_THRESHOLD_ALL;
				if (v.size() <= INSERTION_SORT_THRESHOLD)
				{
					insertionSort(v, 0, v.size());
					return;
				}

				Index len = getMaxLength(v);
				sort_impl(v, len, enableMultiThreading);
			}

			template <unknown T>
			inline void sort_dispatcher(std::vector<T>& v, bool enableMultiThreading)
			{
				static_assert(sizeof(T) == 0,
					"ERROR: Unable to sort vector! CAUSE: Unsupported type!\n");
			}
		}

		// =========================
		// -----T sorted by Key-----
		// =========================

		namespace key
		{
			using namespace shared;
			using shared::getCountVectorThread;
			using shared::getCountVector;

			// =================
			// -----Helpers-----
			// =================

			template <typename T, typename F>
			inline bool isSorted(std::vector<T>& v, F func)
			{
				using Key = invoke_result<T, F>;
				constexpr auto cmpGreater = (is_floating_point<Key>) ?
					[](const Key& a, const Key& b) { return std::strong_order(a, b) > 0; } :
					[](const Key& a, const Key& b) { return a > b; };

				bool sorted = true;

				for (Index i = 0, size = v.size() - 1; i < size; i++)
				{
					if (cmpGreater(func(v[i]), func(v[i + 1])))
					{
						sorted = false;
						break;
					}
				}

				return sorted;
			}

			template <typename T, typename F>
			inline Index getMaxLength(std::vector<T>& v, F func)
			{
				using Key = invoke_result<T, F>;

				Index len = 0;

				if constexpr (std::signed_integral<Key> || is_floating_point<Key>)
				{
					len = sizeof(Key);
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					Key maxVal = func(*std::max_element(v.begin(), v.end(),
						[&func](const T& a, const T& b) {
							return func(a) < func(b);
						}));

					while (maxVal > 0)
					{
						len++;
						maxVal >>= SHIFT_BITS;
					}
				}
				else if constexpr (std::same_as<Key, std::string>)
				{
					len = func(*std::max_element(v.begin(), v.end(),
						[&func](const T& a, const T& b) {
							return func(a).length() < func(b).length();
						})).length();
				}
					
				return len;
			}

			template <typename T, typename F>
			inline void insertionSort(std::vector<T>& v, F func, Index l, Index r)
			{
				using Key = invoke_result<T, F>;

				constexpr auto cmp = (is_floating_point<Key>) ?
					[](const Key& a, const Key& b) { return std::strong_order(a, b) < 0; } :
					[](const Key& a, const Key& b) { return a < b; };

				for (Index i = l + 1; i < r; i++)
				{
					T obj = std::move(v[i]);
					const Key& key = func(obj);
					Index j = i;

					while (j > l && cmp(key, func(v[j - 1])))
					{
						v[j] = std::move(v[j - 1]);
						j--;
					}

					v[j] = std::move(obj);
				}
			}

			template <typename T, known Key>
			inline void insertionSort(std::vector<T>& v, std::vector<Key>& k, Index l, Index r)
			{
				constexpr auto cmp = (is_floating_point<Key>) ?
					[](const Key& a, const Key& b) { return std::strong_order(a, b) < 0; } :
					[](const Key& a, const Key& b) { return a < b; };

				for (Index i = l + 1; i < r; i++)
				{
					T obj = std::move(v[i]);
					Key key = std::move(k[i]);
					Index j = i;

					while (j > l && cmp(key, k[j - 1]))
					{
						v[j] = std::move(v[j - 1]);
						k[j] = std::move(k[j - 1]);
						j--;
					}

					v[j] = std::move(obj);
					k[j] = std::move(key);
				}
			}

			template <typename T, typename F, typename U>
			inline void getConvertedVectorThread(std::vector<T>& v, F func, std::vector<U>& vu, Index l, Index r)
			{
				constexpr Index SIGN_SHIFT = (sizeof(U) * 8) - 1;
				constexpr U SIGN_MASK = 1LL << SIGN_SHIFT;

				for (Index i = l; i < r; i++)
				{
					std::memcpy(&vu[i], &func(v[i]), sizeof(U));
					vu[i] = (vu[i] >> SIGN_SHIFT) ? ~vu[i] : vu[i] ^ SIGN_MASK;
				}
			}

			template <typename T, typename F, typename U>
			inline void getConvertedVector(std::vector<T>& v, F func, std::vector<U>& vu, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				Index threadCount = static_cast<Index>(ceil(SIZE / BUCKET_THRESHOLD));

				if (!enableMultiThreading || threadCount <= 1 || SIZE < SIZE_THRESHOLD)
				{
					getConvertedVectorThread(v, func, vu, 0, SIZE);
				}
				else
				{
					std::vector<std::thread> threads;
					threadCount = std::min(threadCount, static_cast<Index>(std::thread::hardware_concurrency()));
					Index bucketSize = SIZE / threadCount;

					for (Index i = 0; i < threadCount; i++)
					{
						Index start = i * bucketSize;
						Index end = (i == threadCount - 1) ? SIZE : start + bucketSize;
						threads.emplace_back([&v, &func, &vu, start, end]() {
							getConvertedVectorThread(v, func, vu, start, end);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			template <typename T, typename F>
			inline void getCountVectorThread(std::vector<T>& v, F func, std::vector<Index>& count, Index curShiftOrIndex, Index l, Index r)
			{
				using Key = invoke_result<T, F>;

				if constexpr (std::same_as<Key, std::string>)
				{
					for (Index i = l; i < r; i++)
						count[getChar(func(v[i]), curShiftOrIndex)]++;
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					for (Index i = l; i < r; i++)
						count[(func(v[i]) >> curShiftOrIndex) & MASK]++;
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr Index MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (Index i = l; i < r; i++)
							count[(static_cast<t2u<Key>>(func(v[i])) >> curShiftOrIndex) & MASK]++;
					}
					else
					{
						for (Index i = l; i < r; i++)
							count[((static_cast<t2u<Key>>(func(v[i])) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
					}
				}
			}

			template <typename T, typename F>
			inline void getCountVector(std::vector<T>& v, F func, std::vector<Index>& count, Index curShiftOrIndex, Index l, Index r, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				const Index SIZE = r - l;
				Index threadCount = static_cast<Index>(ceil(SIZE / BUCKET_THRESHOLD));

				if (!enableMultiThreading || threadCount <= 1 || SIZE < SIZE_THRESHOLD)
				{
					getCountVectorThread(v, func, count, curShiftOrIndex, l, r);
				}
				else
				{
					constexpr Index ALLOC_SIZE = (std::same_as<Key, std::string>) ? CHARS_ALLOC : BASE;

					threadCount = std::min(threadCount, static_cast<Index>(std::thread::hardware_concurrency()));
					Index bucketSize = SIZE / threadCount;

					std::vector<std::thread> threads;
					std::vector<std::vector<Index>> counts(threadCount, std::vector<Index>(ALLOC_SIZE));

					for (Index i = 0; i < threadCount; i++)
					{
						Index start = l + i * bucketSize;
						Index end = (i == threadCount - 1) ? r : start + bucketSize;
						threads.emplace_back([&v, &func, &counts, i, curShiftOrIndex, start, end]() {
								getCountVectorThread(v, func, counts[i], curShiftOrIndex, start, end);
						});
					}

					for (auto& t : threads)
						t.join();

					for (Index curThread = 0; curThread < threadCount; curThread++)
					{
						for (Index i = 0; i < ALLOC_SIZE; i++)
							count[i] += counts[curThread][i];
					}
				}
			}

			template <typename T, typename F>
			inline void getUpdatedVector(std::vector<T>& v, F func, std::vector<T>& tmp, std::vector<Index>& prefix, Index curShift)
			{
				using Key = invoke_result<T, F>;

				if constexpr (std::unsigned_integral<Key>)
				{
					for (auto& obj : v)
						tmp[prefix[(func(obj) >> curShift) & MASK]++] = std::move(obj);
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr Index MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShift != MAX_SHIFT)
					{
						for (auto& obj : v)
							tmp[prefix[(static_cast<t2u<Key>>(func(obj)) >> curShift) & MASK]++] = std::move(obj);
					}
					else
					{
						for (auto& obj : v)
							tmp[prefix[((static_cast<t2u<Key>>(func(obj)) >> curShift) & MASK) ^ INVERT_MASK]++] = std::move(obj);
					}
				}
			}

			template <typename T, typename F>
			inline void getUpdatedVector(std::vector<T>& v, F func, std::vector<T>& tmp, std::vector<Index>& prefix, Index curShiftOrIndex, Index l, Index r)
			{
				using Key = invoke_result<T, F>;

				if constexpr (is_string<Key>)
				{
					for (Index i = l; i < r; i++)
						tmp[prefix[getChar(func(v[i]), curShiftOrIndex)]++] = std::move(v[i]);
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					for (Index i = l; i < r; i++)
						tmp[prefix[(func(v[i]) >> curShiftOrIndex) & MASK]++] = std::move(v[i]);
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr Index MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (Index i = l; i < r; i++)
							tmp[prefix[(static_cast<t2u<Key>>(func(v[i])) >> curShiftOrIndex) & MASK]++] = std::move(v[i]);
					}
					else
					{
						for (Index i = l; i < r; i++)
							tmp[prefix[((static_cast<t2u<Key>>(func(v[i])) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++] = std::move(v[i]);
					}
				}
			}

			template <typename T, known Key>
			inline void getUpdatedVector(std::vector<T>& v, std::vector<Key>& k, std::vector<T>& tmp, std::vector<Key>& tmpKey, std::vector<Index>& prefix, Index curShiftOrIndex, Index l, Index r)
			{
				if constexpr (is_string<Key>)
				{
					for (Index i = l; i < r; i++)
					{
						Index pos = prefix[getChar(k[i], curShiftOrIndex)]++;
						tmp[pos] = std::move(v[i]);
						tmpKey[pos] = std::move(k[i]);
					}
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					for (Index i = l; i < r; i++)
					{
						Index pos = prefix[(k[i] >> curShiftOrIndex) & MASK]++;
						tmp[pos] = std::move(v[i]);
						tmpKey[pos] = std::move(k[i]);
					}
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr Index MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (Index i = l; i < r; i++)
						{
							Index pos = prefix[(static_cast<t2u<Key>>(k[i]) >> curShiftOrIndex) & MASK]++;
							tmp[pos] = std::move(v[i]);
							tmpKey[pos] = std::move(k[i]);
						}
					}
					else
					{
						for (Index i = l; i < r; i++)
						{
							Index pos = prefix[((static_cast<t2u<Key>>(k[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
							tmp[pos] = std::move(v[i]);
							tmpKey[pos] = std::move(k[i]);
						}
					}
				}
			}

			template <typename T>
			inline void sortByIndicesThread(std::vector<T>& v, std::vector<T>& tmp, std::vector<Index>& indices, Index l, Index r)
			{
				for (Index i = l; i < r; i++)
					tmp[i] = std::move(v[indices[i]]);
			}

			template <typename T>
			inline void sortByIndices(std::vector<T>& v, std::vector<Index>& indices, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				Index threadCount = static_cast<Index>(ceil(SIZE / BUCKET_THRESHOLD));

				if (!enableMultiThreading || threadCount <= 1 || SIZE < SIZE_THRESHOLD)
				{
					std::vector<T> tmp;
					tmp.reserve(SIZE);

					for (const auto& i : indices)
						tmp.emplace_back(std::move(v[i]));

					std::swap(tmp, v);
				}
				else
				{
					std::vector<T> tmp(SIZE);
					std::vector<std::thread> threads;
					threadCount = std::min(threadCount, static_cast<Index>(std::thread::hardware_concurrency()));
					Index bucketSize = SIZE / threadCount;

					for (Index i = 0; i < threadCount; i++)
					{
						Index start = i * bucketSize;
						Index end = (i == threadCount - 1) ? SIZE : start + bucketSize;
						threads.emplace_back([&v, &tmp, &indices, start, end]() {
							sortByIndicesThread(v, tmp, indices, start, end);
						});
					}

					for (auto& t : threads)
						t.join();

					std::swap(tmp, v);
				}
			}

			template <typename T, typename F>
			inline void sortInstance(std::vector<T>& v, F func, std::vector<T>& tmp,
				std::vector<Region>& regions, std::unique_lock<std::mutex>& lkRegions,
				Region initialRegion, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				std::vector<Region> regionsLocal;
				regionsLocal.reserve(v.size() / 100);
				regionsLocal.emplace_back(initialRegion);

				constexpr Index INSERTION_SORT_THRESHOLD = (std::same_as<Key, std::string>) ? INSERTION_SORT_THRESHOLD_STR : INSERTION_SORT_THRESHOLD_ALL;
				constexpr Index ALLOC_SIZE = (std::same_as<Key, std::string>) ? CHARS_ALLOC : BASE;

				while (regionsLocal.size())
				{
					Region region = std::move(regionsLocal.back());
					regionsLocal.pop_back();

					Index l = region.l;
					Index r = region.r;
					Index len = region.len;
					Index curShiftOrIndex = region.curShiftOrIndex;

					if (r - l < 2 || len == 0)
						continue;

					if (r - l <= INSERTION_SORT_THRESHOLD)
					{
						insertionSort(v, func, l, r);
						continue;
					}

					std::vector<Index> count(ALLOC_SIZE);
					std::vector<Index> prefix(ALLOC_SIZE);

					getCountVector(v, func, count, curShiftOrIndex, l, r, enableMultiThreading);
					getPrefixVector<Key>(prefix, count, l);
					getUpdatedVector(v, func, tmp, prefix, curShiftOrIndex, l, r);

					std::move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

					len--;
					if (len == 0)
						continue;

					Index start = 0;
					if constexpr (is_string<Key>)
					{
						start = l + count[256];
						curShiftOrIndex++;
					}
					else
					{
						start = l;
						curShiftOrIndex -= SHIFT_BITS;
					}

					if (!enableMultiThreading)
					{
						for (Index i = 0; i < BASE; i++)
						{
							if (count[i] > 1)
								regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
							start += count[i];
						}
					}
					else
					{
						const Index LOCAL_BUCKET_THRESHOLD = std::max((r - l) / 1000, static_cast<Index>(1000));

						for (Index i = 0; i < CHARS; i++)
						{
							if (count[i] > 1)
							{
								if (count[i] < LOCAL_BUCKET_THRESHOLD)
									regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
								else
								{
									lkRegions.lock();
									regions.emplace_back(start, start + count[i], len, curShiftOrIndex);
									lkRegions.unlock();
								}
							}
							start += count[i];
						}
					}
				}
			}

			template <typename T, known Key>
			inline void sortInstance(std::vector<T>& v, std::vector<Key>& k, 
				std::vector<T>& tmp, std::vector<Key>& tmpKey,
				std::vector<Region>& regions, std::unique_lock<std::mutex>& lkRegions,
				Region initialRegion, bool enableMultiThreading)
			{
				std::vector<Region> regionsLocal;
				regionsLocal.reserve(v.size() / 100);
				regionsLocal.emplace_back(initialRegion);

				constexpr Index INSERTION_SORT_THRESHOLD = (std::same_as<Key, std::string>) ? INSERTION_SORT_THRESHOLD_STR : INSERTION_SORT_THRESHOLD_ALL;
				constexpr Index ALLOC_SIZE = (std::same_as<Key, std::string>) ? CHARS_ALLOC : BASE;

				while (regionsLocal.size())
				{
					Region region = std::move(regionsLocal.back());
					regionsLocal.pop_back();

					Index l = region.l;
					Index r = region.r;
					Index len = region.len;
					Index curShiftOrIndex = region.curShiftOrIndex;

					if (r - l < 2 || len == 0)
						continue;

					if (r - l <= INSERTION_SORT_THRESHOLD)
					{
						insertionSort(v, k, l, r);
						continue;
					}

					std::vector<Index> count(ALLOC_SIZE);
					std::vector<Index> prefix(ALLOC_SIZE);

					getCountVector(k, count, curShiftOrIndex, l, r, enableMultiThreading);
					getPrefixVector<Key>(prefix, count, l);
					getUpdatedVector(v, k, tmp, tmpKey, prefix, curShiftOrIndex, l, r);

					std::move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);
					std::move(tmpKey.begin() + l, tmpKey.begin() + r, k.begin() + l);

					len--;
					if (len == 0)
						continue;

					Index start = 0;
					if constexpr (is_string<T>)
					{
						start = l + count[256];
						curShiftOrIndex++;
					}
					else
					{
						start = l;
						curShiftOrIndex -= SHIFT_BITS;
					}

					if (!enableMultiThreading)
					{
						for (Index i = 0; i < BASE; i++)
						{
							if (count[i] > 1)
								regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
							start += count[i];
						}
					}
					else
					{
						const Index LOCAL_BUCKET_THRESHOLD = std::max((r - l) / 1000, static_cast<Index>(1000));

						for (Index i = 0; i < CHARS; i++)
						{
							if (count[i] > 1)
							{
								if (count[i] < LOCAL_BUCKET_THRESHOLD)
									regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
								else
								{
									lkRegions.lock();
									regions.emplace_back(start, start + count[i], len, curShiftOrIndex);
									lkRegions.unlock();
								}
							}
							start += count[i];
						}
					}
				}
			}

			template <typename T, typename F>
			inline void sortThread(std::vector<T>& v, F func, std::vector<T>& tmp,
				std::vector<Region>& regions, std::mutex& regionsLock,
				Index& runningCounter, Index threadIndex)
			{
				std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

				bool isIdle = false;
				Index iterationsIdle = 0;
				const bool ALLOW_SLEEP = v.size() > ALLOW_SLEEP_THRESHOLD;

				while (true)
				{
					lkRegions.lock();
					if (regions.size())
					{
						Region region = std::move(regions.back());
						regions.pop_back();

						if (isIdle)
						{
							isIdle = false;
							runningCounter++;
							iterationsIdle = 0;
						}

						lkRegions.unlock();
						sortInstance(v, func, tmp, regions, lkRegions, region, 1);
					}
					else
					{
						if (!isIdle)
						{
							isIdle = true;
							runningCounter--;
						}

						if (runningCounter == 0)
						{
							lkRegions.unlock();
							break;
						}

						lkRegions.unlock();
						iterationsIdle++;

						if (ALLOW_SLEEP && iterationsIdle > 100)
						{
							iterationsIdle = 0;
							std::this_thread::sleep_for(std::chrono::nanoseconds(1));
						}
					}
				}
			}

			template <typename T, known Key>
			inline void sortThread(std::vector<T>& v, std::vector<Key>& k, 
				std::vector<T>& tmp, std::vector<Key>& tmpKey,
				std::vector<Region>& regions, std::mutex& regionsLock,
				Index& runningCounter, Index threadIndex)
			{
				std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

				bool isIdle = false;
				Index iterationsIdle = 0;
				const bool ALLOW_SLEEP = v.size() > ALLOW_SLEEP_THRESHOLD;

				while (true)
				{
					lkRegions.lock();
					if (regions.size())
					{
						Region region = std::move(regions.back());
						regions.pop_back();

						if (isIdle)
						{
							isIdle = false;
							runningCounter++;
							iterationsIdle = 0;
						}

						lkRegions.unlock();
						sortInstance(v, k, tmp, tmpKey, regions, lkRegions, region, 1);
					}
					else
					{
						if (!isIdle)
						{
							isIdle = true;
							runningCounter--;
						}

						if (runningCounter == 0)
						{
							lkRegions.unlock();
							break;
						}

						lkRegions.unlock();
						iterationsIdle++;

						if (ALLOW_SLEEP && iterationsIdle > 100)
						{
							iterationsIdle = 0;
							std::this_thread::sleep_for(std::chrono::nanoseconds(1));
						}
					}
				}
			}
				
			// =========================
			// -----Implementations-----
			// =========================

			template <typename T, typename F>
			requires is_small_integral<invoke_result<T, F>>
			inline void sort_impl(std::vector<T>& v, F func, Index len, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;
				
				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				Index curShift = 0;

				while (len--)
				{
					std::vector<Index> count(BASE);
					std::vector<Index> prefix(BASE);

					getCountVector(v, func, count, curShift, 0, SIZE, enableMultiThreading);
					getPrefixVector<Key>(prefix, count, 0);
					getUpdatedVector(v, func, tmp, prefix, curShift);

					std::swap(v, tmp);

					curShift += SHIFT_BITS;
				}
			}

			template <typename T, is_small_integral Key>
			inline void sort_impl(std::vector<T>& v, std::vector<Key>& k, Index len, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				std::vector<Key> tmpKey(SIZE);
				Index curShift = 0;

				while (len--)
				{
					std::vector<Index> count(BASE);
					std::vector<Index> prefix(BASE);

					getCountVector(k, count, curShift, 0, SIZE, enableMultiThreading);
					getPrefixVector<Key>(prefix, count, 0);
					getUpdatedVector(v, k, tmp, tmpKey, prefix, curShift, 0, SIZE);

					std::swap(v, tmp);
					std::swap(k, tmpKey);

					curShift += SHIFT_BITS;
				}
			}

			template <typename T, typename F>
			requires (is_large_integral<invoke_result<T, F>> || is_string<invoke_result<T, F>>)
			inline void sort_impl(std::vector<T>& v, F func, Index len, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				constexpr Index curShiftOrIndex = (is_string<Key>) ? 0 : (sizeof(Key) - 1) * 8;
				const Index LOCAL_BUCKET_THRESHOLD = std::max(static_cast<Index>(SIZE / 1000), static_cast<Index>(1000));

				if (!enableMultiThreading || static_cast<Index>(SIZE / 256) - (LOCAL_BUCKET_THRESHOLD / 10) < LOCAL_BUCKET_THRESHOLD)
				{
					std::mutex tmpMutex;
					std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

					std::vector<Region> tmpVector;
					sortInstance(v, func, tmp, tmpVector, tmpLock, Region(0, SIZE, len, curShiftOrIndex), false);
				}
				else
				{
					Index numOfThreads = std::thread::hardware_concurrency();
					std::vector<std::thread> threads;
					std::mutex regionsLock;
					std::mutex idleLock;
					Index runningCounter = numOfThreads;

					std::vector<Region> regions;
					regions.reserve(SIZE / 1000);
					regions.emplace_back(0, SIZE, len, curShiftOrIndex);
					for (Index i = 0; i < numOfThreads; i++)
					{
						threads.emplace_back([&v, &func, &tmp, &regions, &regionsLock, &runningCounter, i]() {
							sortThread(v, func, tmp, regions, regionsLock, runningCounter, i);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			template <typename T, typename Key>
			requires (is_large_integral<Key> || is_string<Key>)
			inline void sort_impl(std::vector<T>& v, std::vector<Key>& k, Index len, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				std::vector<Key> tmpKey(SIZE);
				constexpr Index curShiftOrIndex = (is_string<Key>) ? 0 : (sizeof(Key) - 1) * 8;
				const Index LOCAL_BUCKET_THRESHOLD = std::max(static_cast<Index>(SIZE / 1000), static_cast<Index>(1000));

				if (!enableMultiThreading || static_cast<Index>(SIZE / 256) - (LOCAL_BUCKET_THRESHOLD / 10) < LOCAL_BUCKET_THRESHOLD)
				{
					std::mutex tmpMutex;
					std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

					std::vector<Region> tmpVector;
					sortInstance(v, k, tmp, tmpKey, tmpVector, tmpLock, Region(0, SIZE, len, curShiftOrIndex), false);
				}
				else
				{
					Index numOfThreads = std::thread::hardware_concurrency();
					std::vector<std::thread> threads;
					std::mutex regionsLock;
					std::mutex idleLock;
					Index runningCounter = numOfThreads;

					std::vector<Region> regions;
					regions.reserve(SIZE / 1000);
					regions.emplace_back(0, SIZE, len, curShiftOrIndex);
					for (Index i = 0; i < numOfThreads; i++)
					{
						threads.emplace_back([&v, &k, &tmp, &tmpKey, &regions, &regionsLock, &runningCounter, i]() {
							sortThread(v, k, tmp, tmpKey, regions, regionsLock, runningCounter, i);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			template <typename T, typename F>
			requires is_floating_point<invoke_result<T, F>>
			inline void sort_impl(std::vector<T>& v, F func, Index len, std::vector<Index>& indices, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				using U = t2u<Key>;
				const Index SIZE = v.size();
				std::vector<U> vu(SIZE);

				getConvertedVector(v, func, vu, enableMultiThreading);
				sort_impl(indices, vu, len, enableMultiThreading);
			}

			// =====================
			// -----Entry Point-----
			// =====================

			template <typename T, typename F>
			requires sort_helper<T, F>
			inline void sort_dispatcher(std::vector<T>& v, F func, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				if constexpr (is_floating_point<Key>)
				{
					static_assert(
						std::numeric_limits<Key>::is_iec559,
						"ERROR: generators.hpp requires IEEE-754/IEC-559 compliance.\n"
					);
				}

				if (isSorted(v, func))
					return;

				Index SIZE = v.size();
				constexpr Index INSERTION_SORT_THRESHOLD = (is_string<Key>) ? INSERTION_SORT_THRESHOLD_STR : INSERTION_SORT_THRESHOLD_ALL;
				if (SIZE <= INSERTION_SORT_THRESHOLD)
				{
					insertionSort(v, func, 0, SIZE);
					return;
				}

				Index len = getMaxLength(v, func);
				const Index COMPLEX_SIZE = sizeof(T);
				const Index INDEX_SIZE = sizeof(Index);

				if constexpr (is_floating_point<Key>)
				{
					std::vector<Index> indices(SIZE);
					std::iota(indices.begin(), indices.end(), static_cast<Index>(0));
					sort_impl(v, func, len, indices, enableMultiThreading);
					sortByIndices(v, indices, enableMultiThreading);
				}
				else
				{
					if (COMPLEX_SIZE <= INDEX_SIZE || len <= 1)
					{
						sort_impl(v, func, len, enableMultiThreading);
					}
					else
					{
						std::vector<Index> indices(SIZE);
						std::iota(indices.begin(), indices.end(), static_cast<Index>(0));
						
						if constexpr (is_string<Key>)
						{
							auto tmpFunc = [&v, &func](const Index& i) -> const Key& { return func(v[i]); };
							sort_impl(indices, tmpFunc, len, enableMultiThreading);
						}
						else
						{
							std::vector<Key> k;
							k.reserve(v.size());

							for (const auto& obj : v)
								k.emplace_back(func(obj));

							sort_impl(indices, k, len, enableMultiThreading);
						}

						sortByIndices(v, indices, enableMultiThreading);
					}
				}
			}

			template <typename T, typename F>
			requires (!sort_helper<T, F>)
			inline void sort_dispatcher(std::vector<T>& v, F func, bool enableMultiThreading)
			{
				static_assert(sizeof(T) == 0,
					"ERROR: Unable to sort vector! CAUSE: Key extractor is invalid or returns an unsupported type!\n");
			}
		}
	}

	// =============
	// -----API-----
	// =============

	template <typename T>
	inline void sort(std::vector<T>& v, bool enableMultiThreading = false)
	{
		internal::value::sort_dispatcher(v, enableMultiThreading);
	}

	template <typename T, typename Proj = std::identity>
	inline void sort(std::vector<T>& v, Proj proj = {}, bool enableMultiThreading = false)
	{
		if constexpr (std::same_as<std::remove_cvref_t<Proj>, std::identity>)
			internal::value::sort_dispatcher(v, enableMultiThreading);
		else
			internal::key::sort_dispatcher(v, proj, enableMultiThreading);
	}
};
