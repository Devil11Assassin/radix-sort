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

			inline constexpr Index SMALL_INTEGRAL_THRESHOLD_BYTES = 1;
			inline constexpr Index LARGE_INTEGRAL_THRESHOLD_BYTES = 8;
			inline constexpr Index INSERTION_SORT_THRESHOLD_STR = 10;
			inline constexpr Index INSERTION_SORT_THRESHOLD_ALL = 100;

			inline const Index MAX_HW_THREADS = static_cast<Index>(std::thread::hardware_concurrency());
			inline constexpr Index MAX_SW_THREADS = 12;
			inline constexpr Index MULTI_THREADING_THRESHOLD = 1'000'000;
			inline constexpr Index SLEEP_ITERATIONS_THRESHOLD = 100;
			inline constexpr Index GLOBAL_BUCKET_THRESHOLD = 10'000;

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
			concept is_small_integral = std::integral<T> && sizeof(T) <= SMALL_INTEGRAL_THRESHOLD_BYTES;

			template <typename T>
			concept is_large_integral = std::integral<T> && sizeof(T) >= LARGE_INTEGRAL_THRESHOLD_BYTES;

			template <typename T>
			concept is_floating_point = std::floating_point<T>;

			template <typename T>
			concept is_string = std::same_as<T, std::string>;

			template <typename T>
			concept supported = std::integral<T> || is_floating_point<T> || is_string<T>;

			template <typename T>
			concept unsupported = !supported<T>;

			template <typename T, typename Proj>
			using invoke_result = std::invoke_result_t<Proj&, const T&>;

			template <typename T, typename Proj>
			using sort_key = std::remove_cvref_t<std::invoke_result_t<Proj&, const T&>>;

			template <typename T, typename Proj>
			concept sortable = requires(Proj proj, const T& t) { 
				{ std::invoke(proj, t) } -> std::same_as<invoke_result<T, Proj>>;
			} && supported<sort_key<T, Proj>>;

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

			inline Index getNumOfThreads(Index n)
			{
				if (n < (MULTI_THREADING_THRESHOLD << 1))
					return 1;

				Index ratio = n / MULTI_THREADING_THRESHOLD;
				Index numOfThreads = 1;
				while ((numOfThreads << 1) <= ratio) 
					numOfThreads <<= 1;

				return std::min({ numOfThreads, MAX_HW_THREADS, MAX_SW_THREADS });
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
				Index numOfThreads = getNumOfThreads(SIZE);

				if (!enableMultiThreading || numOfThreads <= 1)
				{
					getCountVectorThread(v, count, curShiftOrIndex, l, r);
				}
				else
				{
					constexpr Index ALLOC_SIZE = (is_string<T>) ? CHARS_ALLOC : BASE;
					std::vector<std::vector<Index>> counts(numOfThreads, std::vector<Index>(ALLOC_SIZE));
					
					std::vector<std::thread> threads;
					Index bucketSize = SIZE / numOfThreads;

					for (Index i = 0; i < numOfThreads; i++)
					{
						Index start = l + i * bucketSize;
						Index end = (i == numOfThreads - 1) ? r : start + bucketSize;
						threads.emplace_back([&v, &counts, i, curShiftOrIndex, start, end]() {
							getCountVectorThread(v, counts[i], curShiftOrIndex, start, end);
						});
					}

					for (auto& t : threads)
						t.join();

					for (Index curThread = 0; curThread < numOfThreads; curThread++)
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
				if (v.size() < 2)
					return true;

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
			inline void getUnsignedVectorThread(std::vector<T>& v, std::vector<U>& vu, bool reverse, Index l, Index r)
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
			inline void getUnsignedVector(std::vector<T>& v, std::vector<U>& vu, bool reverse, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				Index numOfThreads = getNumOfThreads(SIZE);

				if (!enableMultiThreading || numOfThreads <= 1)
				{
					getUnsignedVectorThread(v, vu, reverse, 0, SIZE);
				}
				else
				{
					std::vector<std::thread> threads;
					Index bucketSize = SIZE / numOfThreads;

					for (Index i = 0; i < numOfThreads; i++)
					{
						Index start = i * bucketSize;
						Index end = (i == numOfThreads - 1) ? SIZE : start + bucketSize;
						threads.emplace_back([&v, &vu, reverse, start, end]() {
							getUnsignedVectorThread(v, vu, reverse, start, end);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			template <typename T>
			inline void getSortedVector(std::vector<T>& v, std::vector<T>& tmp, std::vector<Index>& prefix, Index curShift)
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
			inline void getSortedVector(std::vector<T>& v, std::vector<T>& tmp, std::vector<Index>& prefix, Index curShiftOrIndex, Index l, Index r)
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

			// =========================
			// -----Implementations-----
			// =========================
		
			template <typename T>
			inline void sortLsd(std::vector<T>& v, Index len, bool enableMultiThreading)
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
					getSortedVector(v, tmp, prefix, curShift);

					std::swap(v, tmp);

					curShift += SHIFT_BITS;
				}
			}

			template <typename T>
			inline void sortMsd(std::vector<T>& v, std::vector<T>& tmp,
				std::vector<Region>& regions, std::unique_lock<std::mutex>& lkRegions,
				Region initialRegion, bool enableMultiThreading)
			{
				std::vector<Region> regionsLocal;
				regionsLocal.reserve(v.size() / 100);
				regionsLocal.emplace_back(initialRegion);

				constexpr Index INSERTION_SORT_THRESHOLD = (is_string<T>) ? INSERTION_SORT_THRESHOLD_STR : INSERTION_SORT_THRESHOLD_ALL;
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
					getSortedVector(v, tmp, prefix, curShiftOrIndex, l, r);

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
						for (Index i = 0; i < BASE; i++)
						{
							if (count[i] > 1)
							{
								if (count[i] < GLOBAL_BUCKET_THRESHOLD)
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
			inline void sortMsdThread(std::vector<T>& v, std::vector<T>& tmp,
				std::vector<Region>& regions, std::mutex& regionsLock,
				Index& runningCounter, Index threadIndex)
			{
				std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

				bool isIdle = false;
				Index iterationsIdle = 0;

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
						sortMsd(v, tmp, regions, lkRegions, region, 1);
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

						if (iterationsIdle > SLEEP_ITERATIONS_THRESHOLD)
						{
							iterationsIdle = 0;
							std::this_thread::sleep_for(std::chrono::nanoseconds(1));
						}
					}
				}
			}

			template <typename T>
			inline void sortMsdInit(std::vector<T>& v, Index len, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				constexpr Index curShiftOrIndex = (is_string<T>) ? 0 : (sizeof(T) - 1) * 8;
				Index numOfThreads = getNumOfThreads(SIZE);

				if (!enableMultiThreading || numOfThreads <= 1)
				{
					std::mutex tmpMutex;
					std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

					std::vector<Region> tmpVector;
					sortMsd(v, tmp, tmpVector, tmpLock, Region(0, SIZE, len, curShiftOrIndex), false);
				}
				else
				{
					std::vector<std::thread> threads;
					std::mutex regionsLock;
					Index runningCounter = numOfThreads;

					std::vector<Region> regions;
					regions.reserve(1000);
					regions.emplace_back(0, SIZE, len, curShiftOrIndex);
					
					for (Index i = 0; i < numOfThreads; i++)
					{
						threads.emplace_back([&v, &tmp, &regions, &regionsLock, &runningCounter, i]() {
							sortMsdThread(v, tmp, regions, regionsLock, runningCounter, i);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			// =====================
			// -----Entry Point-----
			// =====================

			template <typename T>
			inline void selectSortStrategy(std::vector<T>& v, bool enableMultiThreading)
			{
				Index len = getMaxLength(v);
				Index numOfThreads = getNumOfThreads(v.size());

				if (is_string<T> || (is_large_integral<T> && len > 1))
					sortMsdInit(v, len, enableMultiThreading);
				else if (is_small_integral<T> || len <= 1 || !enableMultiThreading || numOfThreads <= 1)
					sortLsd(v, len, enableMultiThreading);
				else
					sortMsdInit(v, len, enableMultiThreading);
			}

			template <supported T>
			inline void sortDispatcher(std::vector<T>& v, bool enableMultiThreading)
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

				const Index SIZE = v.size();
				constexpr Index INSERTION_SORT_THRESHOLD = (is_string<T>) ? INSERTION_SORT_THRESHOLD_STR : INSERTION_SORT_THRESHOLD_ALL;
				if (SIZE <= INSERTION_SORT_THRESHOLD)
				{
					insertionSort(v, 0, SIZE);
					return;
				}

				if constexpr (!is_floating_point<T>)
					selectSortStrategy(v, enableMultiThreading);
				else 
				{
					std::vector<t2u<T>> vu(SIZE);
					getUnsignedVector(v, vu, false, enableMultiThreading);
					selectSortStrategy(vu, enableMultiThreading);
					getUnsignedVector(v, vu, true, enableMultiThreading);
				}
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

			template <typename T, typename Proj>
			inline bool isSorted(std::vector<T>& v, Proj proj)
			{
				if (v.size() < 2)
					return true;

				using Key = sort_key<T, Proj>;
				constexpr auto cmpGreater = (is_floating_point<Key>) ?
					[](const Key& a, const Key& b) { return std::strong_order(a, b) > 0; } :
					[](const Key& a, const Key& b) { return a > b; };

				bool sorted = true;

				for (Index i = 0, size = v.size() - 1; i < size; i++)
				{
					if (cmpGreater(std::invoke(proj, v[i]), std::invoke(proj, v[i + 1])))
					{
						sorted = false;
						break;
					}
				}

				return sorted;
			}

			template <typename T, typename Proj>
			inline Index getMaxLength(std::vector<T>& v, Proj proj)
			{
				using Key = sort_key<T, Proj>;

				Index len = 0;

				if constexpr (std::signed_integral<Key> || is_floating_point<Key>)
				{
					len = sizeof(Key);
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					Key maxVal = std::invoke(proj, *std::max_element(v.begin(), v.end(),
						[&proj](const T& a, const T& b) {
							return std::invoke(proj, a) < std::invoke(proj, b);
						}));

					while (maxVal > 0)
					{
						len++;
						maxVal >>= SHIFT_BITS;
					}
				}
				else if constexpr (std::same_as<Key, std::string>)
				{
					len = std::invoke(proj, *std::max_element(v.begin(), v.end(),
						[&proj](const T& a, const T& b) {
							return std::invoke(proj, a).length() < std::invoke(proj, b).length();
						})).length();
				}
					
				return len;
			}

			template <typename T, typename Proj>
			inline void insertionSort(std::vector<T>& v, Proj proj, Index l, Index r)
			{
				using Key = sort_key<T, Proj>;

				constexpr auto cmp = (is_floating_point<Key>) ?
					[](const Key& a, const Key& b) { return std::strong_order(a, b) < 0; } :
					[](const Key& a, const Key& b) { return a < b; };

				for (Index i = l + 1; i < r; i++)
				{
					T obj = std::move(v[i]);
					const Key& key = std::invoke(proj, obj);
					Index j = i;

					while (j > l && cmp(key, std::invoke(proj, v[j - 1])))
					{
						v[j] = std::move(v[j - 1]);
						j--;
					}

					v[j] = std::move(obj);
				}
			}

			template <typename T, typename Key>
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

			template <typename T, typename Proj, typename U>
			inline void getUnsignedVectorThread(std::vector<T>& v, Proj proj, std::vector<U>& vu, Index l, Index r)
			{
				constexpr Index SIGN_SHIFT = (sizeof(U) * 8) - 1;
				constexpr U SIGN_MASK = 1LL << SIGN_SHIFT;

				for (Index i = l; i < r; i++)
				{
					std::memcpy(&vu[i], &std::invoke(proj, v[i]), sizeof(U));
					vu[i] = (vu[i] >> SIGN_SHIFT) ? ~vu[i] : vu[i] ^ SIGN_MASK;
				}
			}

			template <typename T, typename Proj, typename U>
			inline void getUnsignedVector(std::vector<T>& v, Proj proj, std::vector<U>& vu, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				Index numOfThreads = getNumOfThreads(SIZE);

				if (!enableMultiThreading || numOfThreads <= 1)
				{
					getUnsignedVectorThread(v, proj, vu, 0, SIZE);
				}
				else
				{
					std::vector<std::thread> threads;
					Index bucketSize = SIZE / numOfThreads;

					for (Index i = 0; i < numOfThreads; i++)
					{
						Index start = i * bucketSize;
						Index end = (i == numOfThreads - 1) ? SIZE : start + bucketSize;
						threads.emplace_back([&v, &proj, &vu, start, end]() {
							getUnsignedVectorThread(v, proj, vu, start, end);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			template <typename T, typename Proj>
			inline void getCountVectorThread(std::vector<T>& v, Proj proj, std::vector<Index>& count, Index curShiftOrIndex, Index l, Index r)
			{
				using Key = sort_key<T, Proj>;

				if constexpr (std::same_as<Key, std::string>)
				{
					for (Index i = l; i < r; i++)
						count[getChar(std::invoke(proj, v[i]), curShiftOrIndex)]++;
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					for (Index i = l; i < r; i++)
						count[(std::invoke(proj, v[i]) >> curShiftOrIndex) & MASK]++;
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr Index MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (Index i = l; i < r; i++)
							count[(static_cast<t2u<Key>>(std::invoke(proj, v[i])) >> curShiftOrIndex) & MASK]++;
					}
					else
					{
						for (Index i = l; i < r; i++)
							count[((static_cast<t2u<Key>>(std::invoke(proj, v[i])) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
					}
				}
			}

			template <typename T, typename Proj>
			inline void getCountVector(std::vector<T>& v, Proj proj, std::vector<Index>& count, Index curShiftOrIndex, Index l, Index r, bool enableMultiThreading)
			{
				using Key = sort_key<T, Proj>;

				const Index SIZE = r - l;
				Index numOfThreads = getNumOfThreads(SIZE);

				if (!enableMultiThreading || numOfThreads <= 1)
				{
					getCountVectorThread(v, proj, count, curShiftOrIndex, l, r);
				}
				else
				{
					constexpr Index ALLOC_SIZE = (std::same_as<Key, std::string>) ? CHARS_ALLOC : BASE;
					std::vector<std::vector<Index>> counts(numOfThreads, std::vector<Index>(ALLOC_SIZE));

					std::vector<std::thread> threads;
					Index bucketSize = SIZE / numOfThreads;

					for (Index i = 0; i < numOfThreads; i++)
					{
						Index start = l + i * bucketSize;
						Index end = (i == numOfThreads - 1) ? r : start + bucketSize;
						threads.emplace_back([&v, &proj, &counts, i, curShiftOrIndex, start, end]() {
								getCountVectorThread(v, proj, counts[i], curShiftOrIndex, start, end);
						});
					}

					for (auto& t : threads)
						t.join();

					for (Index curThread = 0; curThread < numOfThreads; curThread++)
					{
						for (Index i = 0; i < ALLOC_SIZE; i++)
							count[i] += counts[curThread][i];
					}
				}
			}

			template <typename T, typename Proj>
			inline void getSortedVector(std::vector<T>& v, Proj proj, std::vector<T>& tmp, std::vector<Index>& prefix, Index curShift)
			{
				using Key = sort_key<T, Proj>;

				if constexpr (std::unsigned_integral<Key>)
				{
					for (auto& obj : v)
						tmp[prefix[(std::invoke(proj, obj) >> curShift) & MASK]++] = std::move(obj);
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr Index MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShift != MAX_SHIFT)
					{
						for (auto& obj : v)
							tmp[prefix[(static_cast<t2u<Key>>(std::invoke(proj, obj)) >> curShift) & MASK]++] = std::move(obj);
					}
					else
					{
						for (auto& obj : v)
							tmp[prefix[((static_cast<t2u<Key>>(std::invoke(proj, obj)) >> curShift) & MASK) ^ INVERT_MASK]++] = std::move(obj);
					}
				}
			}

			template <typename T, typename Proj>
			inline void getSortedVector(std::vector<T>& v, Proj proj, std::vector<T>& tmp, std::vector<Index>& prefix, Index curShiftOrIndex, Index l, Index r)
			{
				using Key = sort_key<T, Proj>;

				if constexpr (is_string<Key>)
				{
					for (Index i = l; i < r; i++)
						tmp[prefix[getChar(std::invoke(proj, v[i]), curShiftOrIndex)]++] = std::move(v[i]);
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					for (Index i = l; i < r; i++)
						tmp[prefix[(std::invoke(proj, v[i]) >> curShiftOrIndex) & MASK]++] = std::move(v[i]);
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr Index MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (Index i = l; i < r; i++)
							tmp[prefix[(static_cast<t2u<Key>>(std::invoke(proj, v[i])) >> curShiftOrIndex) & MASK]++] = std::move(v[i]);
					}
					else
					{
						for (Index i = l; i < r; i++)
							tmp[prefix[((static_cast<t2u<Key>>(std::invoke(proj, v[i])) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++] = std::move(v[i]);
					}
				}
			}

			template <typename T, typename Key>
			inline void getSortedVector(std::vector<T>& v, std::vector<Key>& k, std::vector<T>& tmp, std::vector<Key>& tmpKey, std::vector<Index>& prefix, Index curShiftOrIndex, Index l, Index r)
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
				Index numOfThreads = getNumOfThreads(SIZE);

				if (!enableMultiThreading || numOfThreads <= 1)
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
					Index bucketSize = SIZE / numOfThreads;

					for (Index i = 0; i < numOfThreads; i++)
					{
						Index start = i * bucketSize;
						Index end = (i == numOfThreads - 1) ? SIZE : start + bucketSize;
						threads.emplace_back([&v, &tmp, &indices, start, end]() {
							sortByIndicesThread(v, tmp, indices, start, end);
						});
					}

					for (auto& t : threads)
						t.join();

					std::swap(tmp, v);
				}
			}

			// =========================
			// -----Implementations-----
			// =========================

			template <typename T, typename Proj>
			inline void sortLsd(std::vector<T>& v, Proj proj, Index len, bool enableMultiThreading)
			{
				using Key = sort_key<T, Proj>;
				
				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				Index curShift = 0;

				while (len--)
				{
					std::vector<Index> count(BASE);
					std::vector<Index> prefix(BASE);

					getCountVector(v, proj, count, curShift, 0, SIZE, enableMultiThreading);
					getPrefixVector<Key>(prefix, count, 0);
					getSortedVector(v, proj, tmp, prefix, curShift);

					std::swap(v, tmp);

					curShift += SHIFT_BITS;
				}
			}

			template <typename T, typename Key>
			inline void sortLsd(std::vector<T>& v, std::vector<Key>& k, Index len, bool enableMultiThreading)
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
					getSortedVector(v, k, tmp, tmpKey, prefix, curShift, 0, SIZE);

					std::swap(v, tmp);
					std::swap(k, tmpKey);

					curShift += SHIFT_BITS;
				}
			}

			template <typename T, typename Proj>
			inline void sortMsd(std::vector<T>& v, Proj proj, std::vector<T>& tmp,
				std::vector<Region>& regions, std::unique_lock<std::mutex>& lkRegions,
				Region initialRegion, bool enableMultiThreading)
			{
				using Key = sort_key<T, Proj>;

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
						insertionSort(v, proj, l, r);
						continue;
					}

					std::vector<Index> count(ALLOC_SIZE);
					std::vector<Index> prefix(ALLOC_SIZE);

					getCountVector(v, proj, count, curShiftOrIndex, l, r, enableMultiThreading);
					getPrefixVector<Key>(prefix, count, l);
					getSortedVector(v, proj, tmp, prefix, curShiftOrIndex, l, r);

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
						for (Index i = 0; i < BASE; i++)
						{
							if (count[i] > 1)
							{
								if (count[i] < GLOBAL_BUCKET_THRESHOLD)
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

			template <typename T, typename Key>
			inline void sortMsd(std::vector<T>& v, std::vector<Key>& k,
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
					getSortedVector(v, k, tmp, tmpKey, prefix, curShiftOrIndex, l, r);

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
						for (Index i = 0; i < BASE; i++)
						{
							if (count[i] > 1)
							{
								if (count[i] < GLOBAL_BUCKET_THRESHOLD)
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

			template <typename T, typename Proj>
			inline void sortMsdThread(std::vector<T>& v, Proj proj, std::vector<T>& tmp,
				std::vector<Region>& regions, std::mutex& regionsLock,
				Index& runningCounter, Index threadIndex)
			{
				std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

				bool isIdle = false;
				Index iterationsIdle = 0;

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
						sortMsd(v, proj, tmp, regions, lkRegions, region, 1);
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

						if (iterationsIdle > SLEEP_ITERATIONS_THRESHOLD)
						{
							iterationsIdle = 0;
							std::this_thread::sleep_for(std::chrono::nanoseconds(1));
						}
					}
				}
			}

			template <typename T, typename Key>
			inline void sortMsdThread(std::vector<T>& v, std::vector<Key>& k,
				std::vector<T>& tmp, std::vector<Key>& tmpKey,
				std::vector<Region>& regions, std::mutex& regionsLock,
				Index& runningCounter, Index threadIndex)
			{
				std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

				bool isIdle = false;
				Index iterationsIdle = 0;

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
						sortMsd(v, k, tmp, tmpKey, regions, lkRegions, region, 1);
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

						if (iterationsIdle > SLEEP_ITERATIONS_THRESHOLD)
						{
							iterationsIdle = 0;
							std::this_thread::sleep_for(std::chrono::nanoseconds(1));
						}
					}
				}
			}

			template <typename T, typename Proj>
			inline void sortMsdInit(std::vector<T>& v, Proj proj, Index len, bool enableMultiThreading)
			{
				using Key = sort_key<T, Proj>;

				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				constexpr Index curShiftOrIndex = (is_string<Key>) ? 0 : (sizeof(Key) - 1) * 8;
				Index numOfThreads = getNumOfThreads(SIZE);

				if (!enableMultiThreading || numOfThreads <= 0)
				{
					std::mutex tmpMutex;
					std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

					std::vector<Region> tmpVector;
					sortMsd(v, proj, tmp, tmpVector, tmpLock, Region(0, SIZE, len, curShiftOrIndex), false);
				}
				else
				{
					std::vector<std::thread> threads;
					std::mutex regionsLock;
					Index runningCounter = numOfThreads;

					std::vector<Region> regions;
					regions.reserve(1000);
					regions.emplace_back(0, SIZE, len, curShiftOrIndex);
					
					for (Index i = 0; i < numOfThreads; i++)
					{
						threads.emplace_back([&v, &proj, &tmp, &regions, &regionsLock, &runningCounter, i]() {
							sortMsdThread(v, proj, tmp, regions, regionsLock, runningCounter, i);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			template <typename T, typename Key>
			inline void sortMsdInit(std::vector<T>& v, std::vector<Key>& k, Index len, bool enableMultiThreading)
			{
				const Index SIZE = v.size();
				std::vector<T> tmp(SIZE);
				std::vector<Key> tmpKey(SIZE);
				constexpr Index curShiftOrIndex = (is_string<Key>) ? 0 : (sizeof(Key) - 1) * 8;
				Index numOfThreads = getNumOfThreads(SIZE);

				if (!enableMultiThreading || numOfThreads <= 1)
				{
					std::mutex tmpMutex;
					std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

					std::vector<Region> tmpVector;
					sortMsd(v, k, tmp, tmpKey, tmpVector, tmpLock, Region(0, SIZE, len, curShiftOrIndex), false);
				}
				else
				{
					std::vector<std::thread> threads;
					std::mutex regionsLock;
					Index runningCounter = numOfThreads;

					std::vector<Region> regions;
					regions.reserve(1000);
					regions.emplace_back(0, SIZE, len, curShiftOrIndex);

					for (Index i = 0; i < numOfThreads; i++)
					{
						threads.emplace_back([&v, &k, &tmp, &tmpKey, &regions, &regionsLock, &runningCounter, i]() {
							sortMsdThread(v, k, tmp, tmpKey, regions, regionsLock, runningCounter, i);
						});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			// =====================
			// -----Entry Point-----
			// =====================

			template <typename T, typename Proj>
			inline void selectSortStrategy(std::vector<T>& v, Proj proj, Index len, bool enableMultiThreading)
			{
				using Key = sort_key<T, Proj>;
				Index numOfThreads = getNumOfThreads(v.size());

				if (is_string<Key> || (is_large_integral<Key> && len > 1))
					sortMsdInit(v, proj, len, enableMultiThreading);
				else if (is_small_integral<Key> || len <= 1 || !enableMultiThreading || numOfThreads <= 1)
					sortLsd(v, proj, len, enableMultiThreading);
				else
					sortMsdInit(v, proj, len, enableMultiThreading);
			}

			template <typename T, typename Key>
			inline void selectSortStrategy(std::vector<T>& v, std::vector<Key>& k, Index len, bool enableMultiThreading)
			{
				Index numOfThreads = getNumOfThreads(v.size());

				if (is_string<Key> || (is_large_integral<Key> && len > 1))
					sortMsdInit(v, k, len, enableMultiThreading);
				else if (is_small_integral<Key> || len <= 1 || !enableMultiThreading || numOfThreads <= 1)
					sortLsd(v, k, len, enableMultiThreading);
				else
					sortMsdInit(v, k, len, enableMultiThreading);
			}

			template <typename T, typename Proj>
			inline void selectProjImpl(std::vector<T>& v, Proj proj, bool enableMultiThreading)
			{
				using Key = sort_key<T, Proj>;
				const Index SIZE = v.size();
				Index len = getMaxLength(v, proj);
				
				if constexpr (is_floating_point<Key>)
				{
					std::vector<t2u<Key>> vu(SIZE);
					std::vector<Index> indices(SIZE);
					std::iota(indices.begin(), indices.end(), static_cast<Index>(0));

					getUnsignedVector(v, proj, vu, enableMultiThreading);
					selectSortStrategy(indices, vu, len, enableMultiThreading);
					sortByIndices(v, indices, enableMultiThreading);
				}
				else
				{
					constexpr Index COMPLEX_SIZE = sizeof(T);
					constexpr Index INDEX_SIZE = sizeof(Index);

					if (COMPLEX_SIZE <= INDEX_SIZE || len <= 1)
					{
						selectSortStrategy(v, proj, len, enableMultiThreading);
						return;
					}

					std::vector<Index> indices(SIZE);
					std::iota(indices.begin(), indices.end(), static_cast<Index>(0));

					if constexpr (is_string<Key>)
					{
						auto tmpFunc = [&v, &proj](const Index& i) -> const Key& { return std::invoke(proj, v[i]); };
						selectSortStrategy(indices, tmpFunc, len, enableMultiThreading);
					}
					else
					{
						std::vector<Key> k;
						k.reserve(SIZE);

						for (const auto& obj : v)
							k.emplace_back(std::invoke(proj, obj));

						selectSortStrategy(indices, k, len, enableMultiThreading);
					}

					sortByIndices(v, indices, enableMultiThreading);
				}
			}

			template <typename T, typename Proj>
			requires sortable<T, Proj>
			inline void sortDispatcher(std::vector<T>& v, Proj proj, bool enableMultiThreading)
			{
				using Key = sort_key<T, Proj>;

				if constexpr (is_floating_point<Key>)
				{
					static_assert(
						std::numeric_limits<Key>::is_iec559,
						"ERROR: generators.hpp requires IEEE-754/IEC-559 compliance.\n"
					);
				}

				if (isSorted(v, proj))
					return;

				const Index SIZE = v.size();
				constexpr Index INSERTION_SORT_THRESHOLD = (is_string<Key>) ? INSERTION_SORT_THRESHOLD_STR : INSERTION_SORT_THRESHOLD_ALL;
				if (SIZE <= INSERTION_SORT_THRESHOLD)
				{
					insertionSort(v, proj, 0, SIZE);
					return;
				}

				selectProjImpl(v, proj, enableMultiThreading);
			}
		}
	}

	// =============
	// -----API-----
	// =============

	template <typename T, typename Proj = std::identity>
	inline void sort(std::vector<T>& v, Proj proj = {}, bool enableMultiThreading = false)
	{
		if constexpr (std::same_as<std::remove_cvref_t<Proj>, std::identity>)
		{
			static_assert(
				internal::shared::supported<T>,
				"ERROR: Unable to sort vector! CAUSE: Unsupported type!"
			);

			internal::value::sortDispatcher(v, enableMultiThreading);
		}
		else
		{
			static_assert(
				internal::shared::sortable<T, Proj>,
				"ERROR: Unable to sort vector! CAUSE: Key extractor is invalid or returns an unsupported type!"
			);

			internal::key::sortDispatcher(v, proj, enableMultiThreading);
		}
	}
};
