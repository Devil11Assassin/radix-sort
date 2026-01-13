#pragma once
#include <algorithm>
#include <atomic>
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

			inline constexpr int SHIFT_BITS = 8;
			inline constexpr int BASE = 256;
			inline constexpr int MASK = 0xFF;
			inline constexpr int INVERT_MASK = 0x80;

			inline constexpr int SIZE_THRESHOLD = 8'000'000;
			inline constexpr double BUCKET_THRESHOLD = 1'000'000.0;

			inline constexpr int CHARS_ALLOC = 257;
			inline constexpr int CHARS = 256;

			struct RegionIntegral
			{
				int l;
				int r;
				int len;
				RegionIntegral(int l, int r, int len) : l(l), r(r), len(len) {}
			};

			struct RegionString
			{
				int l;
				int r;
				int len;
				int curIndex;
				RegionString(int l, int r, int len, int curIndex) : l(l), r(r), len(len), curIndex(curIndex) {}
			};

			struct RegionNew
			{
				int l;
				int r;
				int len;
				int curShiftOrIndex;
				RegionNew(int l, int r, int len, int curShiftOrIndex) : l(l), r(r), len(len), curShiftOrIndex(curShiftOrIndex) {}
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
			concept sort_key = known<invoke_result<T, F>>;

			template <typename T, typename F>
			concept sort_helper = std::invocable<F, const T&>&& known<invoke_result<T, F>>;
			//concept sort_helper = std::invocable<F, const T&>;

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

			inline int getChar(const std::string& s, int index)
			{
				return (index < s.length()) ? static_cast<unsigned char>(s[index]) : 256;
			}

			template <typename T>
			inline int getMaxLength(std::vector<T>& v)
			{
				int len = 0;

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
			inline void getCountVectorThread(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r)
			{
				if constexpr (is_string<T>)
				{
					for (int i = l; i < r; i++)
						count[getChar(v[i], curShiftOrIndex)]++;
				}
				else if constexpr (std::unsigned_integral<T>)
				{
					for (int i = l; i < r; i++)
						count[(v[i] >> curShiftOrIndex) & MASK]++;
				}
				else if constexpr (std::signed_integral<T>)
				{
					constexpr int MAX_SHIFT = (sizeof(T) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (int i = l; i < r; i++)
							count[(static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK]++;
					}
					else
					{
						for (int i = l; i < r; i++)
							count[((static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
					}
				}
			}

			template <typename T>
			inline void getCountVector(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r, bool enableMultiThreading)
			{
				const int SIZE = r - l;
				int threadCount = static_cast<int>(ceil(SIZE / BUCKET_THRESHOLD));

				if (!enableMultiThreading || threadCount <= 1 || SIZE < SIZE_THRESHOLD)
				{
					getCountVectorThread(v, count, curShiftOrIndex, l, r);
				}
				else
				{
					constexpr int ALLOC_SIZE = (is_string<T>) ? CHARS_ALLOC : BASE;

					threadCount = std::min(threadCount, static_cast<int>(std::thread::hardware_concurrency()));
					int bucketSize = SIZE / threadCount;

					std::vector<std::thread> threads;
					std::vector<std::vector<int>> counts(threadCount, std::vector<int>(ALLOC_SIZE));

					for (int i = 0; i < threadCount; i++)
					{
						int start = l + i * bucketSize;
						int end = (i == threadCount - 1) ? r : start + bucketSize;
						threads.emplace_back(getCountVectorThread<T>, std::ref(v), std::ref(counts[i]), curShiftOrIndex, start, end);
					}

					for (auto& t : threads)
						t.join();

					for (int curThread = 0; curThread < threadCount; curThread++)
					{
						for (int i = 0; i < ALLOC_SIZE; i++)
							count[i] += counts[curThread][i];
					}
				}
			}

			template <typename T>
			inline void getPrefixVector(std::vector<int>& prefix, std::vector<int>& count, int l)
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

				for (int i = 1; i < BASE; i++)
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
			inline bool isSorted(std::vector<T>& v)
			{
				constexpr auto cmpLess = (is_floating_point<T>) ?
					[](const T& a, const T& b) { return std::strong_order(a, b) < 0; } :
					[](const T& a, const T& b) { return a < b; };

				constexpr auto cmpGreater = (is_floating_point<T>) ?
					[](const T& a, const T& b) { return std::strong_order(a, b) > 0; } :
					[](const T& a, const T& b) { return a > b; };

				bool sortedAsc = true;
				bool sortedDesc = true;

				for (int i = 0, size = v.size() - 1; i < size; i++)
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
			inline void insertionSort(std::vector<T>& v, int l, int r)
			{
				constexpr auto cmp = (is_floating_point<T>) ?
					[](const T& a, const T& b) { return std::strong_order(a, b) < 0; } :
					[](const T& a, const T& b) { return a < b; };

				for (int i = l + 1; i < r; i++)
				{
					T val = std::move(v[i]);
					int j = i - 1;

					while (j >= l && cmp(val, v[j]))
					{
						v[j + 1] = std::move(v[j]);
						j--;
					}

					v[j + 1] = std::move(val);
				}
			}

			template <typename T, typename U>
			inline void getConvertedVectorThread(std::vector<T>& v, std::vector<U>& vu, bool reverse, int l, int r)
			{
				constexpr int SIGN_SHIFT = (sizeof(T) * 8) - 1;
				constexpr U SIGN_MASK = 1LL << SIGN_SHIFT;

				if (!reverse)
				{
					for (int i = l; i < r; i++)
					{
						std::memcpy(&vu[i], &v[i], sizeof(T));
						vu[i] = (vu[i] >> SIGN_SHIFT) ? ~vu[i] : vu[i] ^ SIGN_MASK;
					}
				}
				else
				{
					for (int i = l; i < r; i++)
					{
						vu[i] = (vu[i] >> SIGN_SHIFT) ? vu[i] ^ SIGN_MASK : ~vu[i];
						std::memcpy(&v[i], &vu[i], sizeof(T));
					}
				}
			}

			template <typename T, typename U>
			inline void getConvertedVector(std::vector<T>& v, std::vector<U>& vu, bool reverse, bool enableMultiThreading)
			{
				const int SIZE = v.size();
				int threadCount = static_cast<int>(ceil(SIZE / BUCKET_THRESHOLD));

				if (!enableMultiThreading || threadCount <= 1 || SIZE < SIZE_THRESHOLD)
				{
					getConvertedVectorThread(v, vu, reverse, 0, SIZE);
				}
				else
				{
					std::vector<std::thread> threads;
					threadCount = std::min(threadCount, static_cast<int>(std::thread::hardware_concurrency()));
					int bucketSize = SIZE / threadCount;

					for (int i = 0; i < threadCount; i++)
					{
						int start = i * bucketSize;
						int end = (i == threadCount - 1) ? SIZE : start + bucketSize;
						threads.emplace_back([&v, &vu, reverse, start, end]() {
							getConvertedVectorThread(v, vu, reverse, start, end);
							});
					}

					for (auto& t : threads)
						t.join();
				}
			}

			template <typename T>
			inline void getUpdatedVector(std::vector<T>& v, std::vector<T>& tmp, std::vector<int>& prefix, int curShift)
			{
				if constexpr (std::unsigned_integral<T>)
				{
					for (const auto& num : v)
						tmp[prefix[(num >> curShift) & MASK]++] = num;
				}
				else if constexpr (std::signed_integral<T>)
				{
					constexpr int MAX_SHIFT = (sizeof(T) - 1) * 8;

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
			inline void getUpdatedVector(std::vector<T>& v, std::vector<T>& tmp, std::vector<int>& prefix, int curShiftOrIndex, int l, int r)
			{
				if constexpr (is_string<T>)
				{
					for (int i = l; i < r; i++)
						tmp[prefix[getChar(v[i], curShiftOrIndex)]++] = std::move(v[i]);
				}
				else if constexpr (std::unsigned_integral<T>)
				{
					for (int i = l; i < r; i++)
						tmp[prefix[(v[i] >> curShiftOrIndex) & MASK]++] = v[i];
				}
				else if constexpr (std::signed_integral<T>)
				{
					constexpr int MAX_SHIFT = (sizeof(T) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (int i = l; i < r; i++)
							tmp[prefix[(static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK]++] = v[i];
					}
					else
					{
						for (int i = l; i < r; i++)
							tmp[prefix[((static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++] = v[i];
					}
				}
			}

			template <typename T>
			inline void sortInstance(std::vector<T>& v, std::vector<T>& tmp,
				std::vector<RegionNew>& regions, std::unique_lock<std::mutex>& lkRegions,
				RegionNew initialRegion, bool enableMultiThreading)
			{
				std::vector<RegionNew> regionsLocal;
				regionsLocal.reserve(v.size() / 100);
				regionsLocal.emplace_back(initialRegion);

				constexpr int INSERTION_SORT_THRESHOLD = (is_string<T>) ? 10 : 100;
				constexpr int ALLOC_SIZE = (is_string<T>) ? CHARS_ALLOC : BASE;

				while (regionsLocal.size())
				{
					RegionNew region = std::move(regionsLocal.back());
					regionsLocal.pop_back();

					int l = region.l;
					int r = region.r;
					int len = region.len;
					int curShiftOrIndex = region.curShiftOrIndex;

					if (r - l < 2 || len == 0)
						continue;

					if (r - l <= INSERTION_SORT_THRESHOLD)
					{
						insertionSort(v, l, r);
						continue;
					}

					std::vector<int> count(ALLOC_SIZE);
					std::vector<int> prefix(ALLOC_SIZE);

					getCountVector(v, count, curShiftOrIndex, l, r, enableMultiThreading);
					getPrefixVector<T>(prefix, count, l);
					getUpdatedVector(v, tmp, prefix, curShiftOrIndex, l, r);

					std::move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

					len--;
					if (len == 0)
						continue;

					int start = 0;
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
						for (int i = 0; i < BASE; i++)
						{
							if (count[i] > 1)
								regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
							start += count[i];
						}
					}
					else
					{
						const int LOCAL_BUCKET_THRESHOLD = std::max((r - l) / 1000, 1000);

						for (int i = 0; i < CHARS; i++)
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
				std::vector<RegionNew>& regions, std::mutex& regionsLock,
				int& runningCounter, int threadIndex)
			{
				std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

				bool isIdle = false;
				int iterationsIdle = 0;
				const bool ALLOW_SLEEP = v.size() > 1e6;

				while (true)
				{
					lkRegions.lock();
					if (regions.size())
					{
						RegionNew region = std::move(regions.back());
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

			// =====================================
			// -----Implementation Declarations-----
			// =====================================
		
			template <is_small_integral T>
			inline void sort_impl(std::vector<T>&, bool);

			template <is_floating_point T>
			inline void sort_impl(std::vector<T>&, bool);

			template <typename T> requires (is_large_integral<T> || is_string<T>)
			inline void sort_impl(std::vector<T>&, bool);
		
			// ====================================
			// -----Implementation Definitions-----
			// ====================================
		
			template <is_small_integral T>
			inline void sort_impl(std::vector<T>& v, bool enableMultiThreading)
			{
				const int SIZE = v.size();
				std::vector<T> tmp(SIZE);
				int len = getMaxLength(v);
				int curShift = 0;

				while (len--)
				{
					std::vector<int> count(BASE);
					std::vector<int> prefix(BASE);

					getCountVector(v, count, curShift, 0, SIZE, enableMultiThreading);
					getPrefixVector<T>(prefix, count, 0);
					getUpdatedVector(v, tmp, prefix, curShift);

					std::swap(v, tmp);

					curShift += SHIFT_BITS;
				}
			}

			template <is_floating_point T>
			inline void sort_impl(std::vector<T>& v, bool enableMultiThreading)
			{
				using U = t2u<T>;
				const int SIZE = v.size();
				std::vector<U> vu(SIZE);

				getConvertedVector(v, vu, false, enableMultiThreading);
				sort_impl<U>(vu, enableMultiThreading);
				getConvertedVector(v, vu, true, enableMultiThreading);
			}

			template <typename T> requires (is_large_integral<T> || is_string<T>)
			inline void sort_impl(std::vector<T>& v, bool enableMultiThreading)
			{
				const int SIZE = v.size();
				std::vector<T> tmp(SIZE);
				int len = getMaxLength(v);
				constexpr int curShiftOrIndex = (is_string<T>) ? 0 : (sizeof(T) - 1) * 8;
				const int LOCAL_BUCKET_THRESHOLD = std::max(static_cast<int>(SIZE / 1000), 1000);

				if (!enableMultiThreading || static_cast<int>(SIZE / 256) - (LOCAL_BUCKET_THRESHOLD / 10) < LOCAL_BUCKET_THRESHOLD)
				{
					std::mutex tmpMutex;
					std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

					std::vector<RegionNew> tmpVector;
					sortInstance(v, tmp, tmpVector, tmpLock, RegionNew(0, SIZE, len, curShiftOrIndex), false);
				}
				else
				{
					int numOfThreads = std::thread::hardware_concurrency();
					std::vector<std::thread> threads;
					std::mutex regionsLock;
					std::mutex idleLock;
					int runningCounter = numOfThreads;

					std::vector<RegionNew> regions;
					regions.reserve(SIZE / 1000);
					regions.emplace_back(0, SIZE, len, curShiftOrIndex);
					for (int i = 0; i < numOfThreads; i++)
					{
						threads.emplace_back([&v, &tmp, &regions, &regionsLock, &runningCounter, i]() {
							sortThread(v, tmp, regions, regionsLock, runningCounter, i);
						});
					}

					for (auto& t : threads)
						t.join();
				}
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

				if (isSorted(v))
					return;

				constexpr int INSERTION_SORT_THRESHOLD = (is_string<T>) ? 10 : 100;
				if (v.size() <= INSERTION_SORT_THRESHOLD)
				{
					insertionSort(v, 0, v.size());
					return;
				}

				sort_impl(v, enableMultiThreading);
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
			using shared::getMaxLength;
			using shared::getCountVectorThread;
			using shared::getCountVector;

			// =================
			// -----Helpers-----
			// =================

			template <typename T, typename F>
			inline int getMaxLength(std::vector<T>& v, F func)
			{
				using Key = invoke_result<T, F>;

				int len = 0;

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
			inline void insertionSort(std::vector<T>& v, F func, int l, int r)
			{
				using Key = invoke_result<T, F>;

				constexpr auto cmp = (is_floating_point<Key>) ?
					[](const Key& a, const Key& b) { return std::strong_order(a, b) < 0; } :
					[](const Key& a, const Key& b) { return a < b; };

				for (int i = l + 1; i < r; i++)
				{
					T obj = std::move(v[i]);
					const Key& key = func(obj);
					int j = i - 1;

					while (j >= l && cmp(key, func(v[j])))
					{
						v[j + 1] = std::move(v[j]);
						j--;
					}

					v[j + 1] = std::move(obj);
				}
			}

			template <typename T, known Key>
			inline void insertionSort(std::vector<T>& v, std::vector<Key>& k, int l, int r)
			{
				constexpr auto cmp = (is_floating_point<Key>) ?
					[](const Key& a, const Key& b) { return std::strong_order(a, b) < 0; } :
					[](const Key& a, const Key& b) { return a < b; };

				for (int i = l + 1; i < r; i++)
				{
					T obj = std::move(v[i]);
					Key key = std::move(k[i]);
					int j = i - 1;

					while (j >= l && cmp(key, k[j]))
					{
						v[j + 1] = std::move(v[j]);
						k[j + 1] = std::move(k[j]);
						j--;
					}

					v[j + 1] = std::move(obj);
					k[j + 1] = std::move(key);
				}
			}

			template <typename T, typename F>
			inline void getCountVectorThread(std::vector<T>& v, F func, std::vector<int>& count, int curShiftOrIndex, int l, int r)
			{
				using Key = invoke_result<T, F>;

				if constexpr (std::same_as<Key, std::string>)
				{
					for (int i = l; i < r; i++)
						count[getChar(func(v[i]), curShiftOrIndex)]++;
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					for (int i = l; i < r; i++)
						count[(func(v[i]) >> curShiftOrIndex) & MASK]++;
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr int MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (int i = l; i < r; i++)
							count[(static_cast<t2u<Key>>(func(v[i])) >> curShiftOrIndex) & MASK]++;
					}
					else
					{
						for (int i = l; i < r; i++)
							count[((static_cast<t2u<Key>>(func(v[i])) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
					}
				}
			}

			template <typename T, typename F>
			inline void getCountVector(std::vector<T>& v, F func, std::vector<int>& count, int curShiftOrIndex, int l, int r, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				const int SIZE = r - l;
				int threadCount = static_cast<int>(ceil(SIZE / BUCKET_THRESHOLD));

				if (!enableMultiThreading || threadCount <= 1 || SIZE < SIZE_THRESHOLD)
				{
					getCountVectorThread(v, func, count, curShiftOrIndex, l, r);
				}
				else
				{
					constexpr int ALLOC_SIZE = (std::same_as<Key, std::string>) ? CHARS_ALLOC : BASE;

					threadCount = std::min(threadCount, static_cast<int>(std::thread::hardware_concurrency()));
					int bucketSize = SIZE / threadCount;

					std::vector<std::thread> threads;
					std::vector<std::vector<int>> counts(threadCount, std::vector<int>(ALLOC_SIZE));

					for (int i = 0; i < threadCount; i++)
					{
						int start = l + i * bucketSize;
						int end = (i == threadCount - 1) ? r : start + bucketSize;
						threads.emplace_back([&v, &func, &counts, i, curShiftOrIndex, start, end]() {
								getCountVectorThread(v, func, counts[i], curShiftOrIndex, start, end);
						});
					}

					for (auto& t : threads)
						t.join();

					for (int curThread = 0; curThread < threadCount; curThread++)
					{
						for (int i = 0; i < ALLOC_SIZE; i++)
							count[i] += counts[curThread][i];
					}
				}
			}

			template <typename T, typename F>
			inline void getUpdatedVector(std::vector<T>& v, F func, std::vector<T>& tmp, std::vector<int>& prefix, int curShift)
			{
				using Key = invoke_result<T, F>;

				if constexpr (std::unsigned_integral<Key>)
				{
					for (auto& obj : v)
						tmp[prefix[(func(obj) >> curShift) & MASK]++] = std::move(obj);
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr int MAX_SHIFT = (sizeof(Key) - 1) * 8;

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
			inline void getUpdatedVector(std::vector<T>& v, F func, std::vector<T>& tmp, std::vector<int>& prefix, int curShiftOrIndex, int l, int r)
			{
				using Key = invoke_result<T, F>;

				if constexpr (is_string<Key>)
				{
					for (int i = l; i < r; i++)
						tmp[prefix[getChar(func(v[i]), curShiftOrIndex)]++] = std::move(v[i]);
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					for (int i = l; i < r; i++)
						tmp[prefix[(func(v[i]) >> curShiftOrIndex) & MASK]++] = std::move(v[i]);
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr int MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (int i = l; i < r; i++)
							tmp[prefix[(static_cast<t2u<Key>>(func(v[i])) >> curShiftOrIndex) & MASK]++] = std::move(v[i]);
					}
					else
					{
						for (int i = l; i < r; i++)
							tmp[prefix[((static_cast<t2u<Key>>(func(v[i])) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++] = std::move(v[i]);
					}
				}
			}

			template <typename T, known Key>
			inline void getUpdatedVector(std::vector<T>& v, std::vector<Key>& k, std::vector<T>& tmp, std::vector<Key>& tmpKey, std::vector<int>& prefix, int curShiftOrIndex, int l, int r)
			{
				if constexpr (is_string<Key>)
				{
					for (int i = l; i < r; i++)
					{
						int pos = prefix[getChar(k[i], curShiftOrIndex)]++;
						tmp[pos] = std::move(v[i]);
						tmpKey[pos] = std::move(k[i]);
					}
				}
				else if constexpr (std::unsigned_integral<Key>)
				{
					for (int i = l; i < r; i++)
					{
						int pos = prefix[(k[i] >> curShiftOrIndex) & MASK]++;
						tmp[pos] = std::move(v[i]);
						tmpKey[pos] = std::move(k[i]);
					}
				}
				else if constexpr (std::signed_integral<Key>)
				{
					constexpr int MAX_SHIFT = (sizeof(Key) - 1) * 8;

					if (curShiftOrIndex != MAX_SHIFT)
					{
						for (int i = l; i < r; i++)
						{
							int pos = prefix[(static_cast<t2u<Key>>(k[i]) >> curShiftOrIndex) & MASK]++;
							tmp[pos] = std::move(v[i]);
							tmpKey[pos] = std::move(k[i]);
						}
					}
					else
					{
						for (int i = l; i < r; i++)
						{
							int pos = prefix[((static_cast<t2u<Key>>(k[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
							tmp[pos] = std::move(v[i]);
							tmpKey[pos] = std::move(k[i]);
						}
					}
				}
			}

			template <typename T>
			inline void sortByIndicesThread(std::vector<T>& v, std::vector<T>& tmp, std::vector<int>& indices, int l, int r)
			{
				for (int i = l; i < r; i++)
					tmp[i] = std::move(v[indices[i]]);
			}

			template <typename T>
			inline void sortByIndices(std::vector<T>& v, std::vector<int>& indices, bool enableMultiThreading)
			{
				const int SIZE = v.size();

				if (!enableMultiThreading)
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
					int threadCount = std::thread::hardware_concurrency();
					int bucketSize = SIZE / threadCount;

					for (int i = 0; i < threadCount; i++)
					{
						int start = i * bucketSize;
						int end = (i == threadCount - 1) ? SIZE : start + bucketSize;
						threads.emplace_back([&v, &tmp, &indices, start, end]() {
							sortByIndicesThread(v, tmp, indices, start, end);
						});
					}

					for (auto& t : threads)
						t.join();

					std::swap(tmp, v);
				}
			}

			template <typename T, typename F, typename TRegion>
			inline void sortInstance(std::vector<T>& v, F func, std::vector<T>& tmp,
				std::vector<TRegion>& regions, std::unique_lock<std::mutex>& lkRegions,
				TRegion initialRegion, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				std::vector<TRegion> regionsLocal;
				regionsLocal.reserve(v.size() / 100);
				regionsLocal.emplace_back(initialRegion);

				constexpr int INSERTION_SORT_THRESHOLD = (std::same_as<Key, std::string>) ? 10 : 100;
				constexpr int ALLOC_SIZE = (std::same_as<Key, std::string>) ? CHARS_ALLOC : BASE;

				while (regionsLocal.size())
				{
					TRegion region = std::move(regionsLocal.back());
					regionsLocal.pop_back();

					int l = region.l;
					int r = region.r;
					int len = region.len;
					int curShiftOrIndex;

					if constexpr (std::same_as<TRegion, RegionString>)
						curShiftOrIndex = region.curIndex;
					else
						curShiftOrIndex = (len - 1) * SHIFT_BITS;

					if (r - l < 2 || len == 0)
						continue;

					if (r - l <= INSERTION_SORT_THRESHOLD)
					{
						insertionSort(v, func, l, r);
						continue;
					}

					std::vector<int> count(ALLOC_SIZE);
					std::vector<int> prefix(ALLOC_SIZE);

					getCountVector(v, func, count, curShiftOrIndex, l, r, enableMultiThreading);
					getPrefixVector<Key>(prefix, count, l);
					getUpdatedVector(v, func, tmp, prefix, curShiftOrIndex, l, r);

					std::move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

					len--;
					if (len == 0)
						continue;

					if constexpr (std::same_as<Key, std::string>)
						curShiftOrIndex++;

					if (!enableMultiThreading)
					{
						if constexpr (std::same_as<Key, std::string>)
						{
							for (int i = 0, start = l + count[256]; i < CHARS; i++)
							{
								if (count[i] > 1)
									regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
								start += count[i];
							}
						}
						else
						{
							for (int i = 0, start = l; i < BASE; i++)
							{
								if (count[i] > 1)
									regionsLocal.emplace_back(start, start + count[i], len);
								start += count[i];
							}
						}
					}
					else
					{
						const int LOCAL_BUCKET_THRESHOLD = std::max((r - l) / 1000, 1000);

						if constexpr (std::same_as<Key, std::string>)
						{
							for (int i = 0, start = l + count[256]; i < CHARS; i++)
							{
								if (count[i] < LOCAL_BUCKET_THRESHOLD)
									regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
								else
								{
									lkRegions.lock();
									regions.emplace_back(start, start + count[i], len, curShiftOrIndex);
									lkRegions.unlock();
								}
								start += count[i];
							}
						}
						else
						{
							for (int i = 0, start = l; i < BASE; i++)
							{
								if (count[i] < LOCAL_BUCKET_THRESHOLD)
									regionsLocal.emplace_back(start, start + count[i], len);
								else
								{
									lkRegions.lock();
									regions.emplace_back(start, start + count[i], len);
									lkRegions.unlock();
								}
								start += count[i];
							}
						}
					}
				}
			}

			template <typename T, known Key, typename TRegion>
			inline void sortInstance(std::vector<T>& v, std::vector<Key>& k, 
				std::vector<T>& tmp, std::vector<Key>& tmpKey,
				std::vector<TRegion>& regions, std::unique_lock<std::mutex>& lkRegions,
				TRegion initialRegion, bool enableMultiThreading)
			{
				std::vector<TRegion> regionsLocal;
				regionsLocal.reserve(v.size() / 100);
				regionsLocal.emplace_back(initialRegion);

				constexpr int INSERTION_SORT_THRESHOLD = (std::same_as<Key, std::string>) ? 10 : 100;
				constexpr int ALLOC_SIZE = (std::same_as<Key, std::string>) ? CHARS_ALLOC : BASE;

				while (regionsLocal.size())
				{
					TRegion region = std::move(regionsLocal.back());
					regionsLocal.pop_back();

					int l = region.l;
					int r = region.r;
					int len = region.len;
					int curShiftOrIndex;

					if constexpr (std::same_as<TRegion, RegionString>)
						curShiftOrIndex = region.curIndex;
					else
						curShiftOrIndex = (len - 1) * SHIFT_BITS;

					if (r - l < 2 || len == 0)
						continue;

					if (r - l <= INSERTION_SORT_THRESHOLD)
					{
						insertionSort(v, k, l, r);
						continue;
					}

					std::vector<int> count(ALLOC_SIZE);
					std::vector<int> prefix(ALLOC_SIZE);

					getCountVector(k, count, curShiftOrIndex, l, r, enableMultiThreading);
					getPrefixVector<Key>(prefix, count, l);
					getUpdatedVector(v, k, tmp, tmpKey, prefix, curShiftOrIndex, l, r);

					std::move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);
					std::move(tmpKey.begin() + l, tmpKey.begin() + r, k.begin() + l);

					len--;
					if (len == 0)
						continue;

					if constexpr (std::same_as<Key, std::string>)
						curShiftOrIndex++;

					if (!enableMultiThreading)
					{
						if constexpr (std::same_as<Key, std::string>)
						{
							for (int i = 0, start = l + count[256]; i < CHARS; i++)
							{
								if (count[i] > 1)
									regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
								start += count[i];
							}
						}
						else
						{
							for (int i = 0, start = l; i < BASE; i++)
							{
								if (count[i] > 1)
									regionsLocal.emplace_back(start, start + count[i], len);
								start += count[i];
							}
						}
					}
					else
					{
						const int LOCAL_BUCKET_THRESHOLD = std::max((r - l) / 1000, 1000);

						if constexpr (std::same_as<Key, std::string>)
						{
							for (int i = 0, start = l + count[256]; i < CHARS; i++)
							{
								if (count[i] < LOCAL_BUCKET_THRESHOLD)
									regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
								else
								{
									lkRegions.lock();
									regions.emplace_back(start, start + count[i], len, curShiftOrIndex);
									lkRegions.unlock();
								}
								start += count[i];
							}
						}
						else
						{
							for (int i = 0, start = l; i < BASE; i++)
							{
								if (count[i] < LOCAL_BUCKET_THRESHOLD)
									regionsLocal.emplace_back(start, start + count[i], len);
								else
								{
									lkRegions.lock();
									regions.emplace_back(start, start + count[i], len);
									lkRegions.unlock();
								}
								start += count[i];
							}
						}
					}
				}
			}

			template <typename T, typename F, typename TRegion>
			inline void sortThread(std::vector<T>& v, F func, std::vector<T>& tmp,
				std::vector<TRegion>& regions, std::mutex& regionsLock,
				std::atomic<int>& runningCounter, int threadIndex)
			{
				std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

				bool isIdle = false;
				int iterationsIdle = 0;
				const bool ALLOW_SLEEP = v.size() > 1e6;

				while (true)
				{
					lkRegions.lock();
					if (regions.size())
					{
						TRegion region = std::move(regions.back());
						regions.pop_back();
						lkRegions.unlock();

						if (isIdle)
						{
							isIdle = false;
							runningCounter++;
							iterationsIdle = 0;
						}

						sortInstance(v, func, tmp, regions, lkRegions, region, 1);
					}
					else
					{
						lkRegions.unlock();

						if (!isIdle)
						{
							isIdle = true;
							runningCounter--;
						}

						if (runningCounter.load() == 0)
							break;

						iterationsIdle++;

						if (ALLOW_SLEEP && iterationsIdle > 100)
						{
							iterationsIdle = 0;
							std::this_thread::sleep_for(std::chrono::nanoseconds(1));
						}
					}
				}
			}

			template <typename T, known Key, typename TRegion>
			inline void sortThread(std::vector<T>& v, std::vector<Key>& k, 
				std::vector<T>& tmp, std::vector<Key>& tmpKey,
				std::vector<TRegion>& regions, std::mutex& regionsLock,
				std::atomic<int>& runningCounter, int threadIndex)
			{
				std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

				bool isIdle = false;
				int iterationsIdle = 0;
				const bool ALLOW_SLEEP = v.size() > 1e6;

				while (true)
				{
					lkRegions.lock();
					if (regions.size())
					{
						TRegion region = std::move(regions.back());
						regions.pop_back();
						lkRegions.unlock();

						if (isIdle)
						{
							isIdle = false;
							runningCounter++;
							iterationsIdle = 0;
						}

						sortInstance(v, k, tmp, tmpKey, regions, lkRegions, region, 1);
					}
					else
					{
						lkRegions.unlock();

						if (!isIdle)
						{
							isIdle = true;
							runningCounter--;
						}

						if (runningCounter.load() == 0)
							break;

						iterationsIdle++;

						if (ALLOW_SLEEP && iterationsIdle > 100)
						{
							iterationsIdle = 0;
							std::this_thread::sleep_for(std::chrono::nanoseconds(1));
						}
					}
				}
			}

			// =====================================
			// -----Implementation Declarations-----
			// =====================================

			template <typename T, typename F> 
			requires is_small_integral<invoke_result<T, F>>
			inline void sort_impl(std::vector<T>&, F, bool);

			template <typename T, is_small_integral Key> 
			inline void sort_impl(std::vector<T>&, std::vector<Key>&, bool);
			
			template <typename T, typename F>
			requires is_floating_point<invoke_result<T, F>>
			inline void sort_impl(std::vector<T>&, F, bool);

			template <typename T, typename F>
			requires (is_large_integral<invoke_result<T, F>> || is_string<invoke_result<T, F>>)
			inline void sort_impl(std::vector<T>&, F, bool);

			template <typename T, typename Key>
			requires (is_large_integral<Key> || is_string<Key>)
			inline void sort_impl(std::vector<T>&, std::vector<Key>&, bool);
				
			// ====================================
			// -----Implementation Definitions-----
			// ====================================

			template <typename T, typename F>
			requires is_small_integral<invoke_result<T, F>>
			inline void sort_impl(std::vector<T>& v, F func, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				int len = getMaxLength(v, func);

				std::vector<T> tmp(v.size());
				int curShift = 0;

				while (len--)
				{
					std::vector<int> count(BASE);
					std::vector<int> prefix(BASE);

					getCountVector(v, func, count, curShift, 0, v.size(), enableMultiThreading);
					getPrefixVector<Key>(prefix, count, 0);
					getUpdatedVector(v, func, tmp, prefix, curShift);

					std::swap(v, tmp);

					curShift += SHIFT_BITS;
				}
			}

			template <typename T, is_small_integral Key>
			inline void sort_impl(std::vector<T>& v, std::vector<Key>& k, bool enableMultiThreading)
			{
				const int SIZE = v.size();

				int len = getMaxLength(k);

				std::vector<T> tmp(SIZE);
				std::vector<Key> tmpKey(SIZE);
				int curShift = 0;

				while (len--)
				{
					std::vector<int> count(BASE);
					std::vector<int> prefix(BASE);

					getCountVector(k, count, curShift, 0, SIZE, enableMultiThreading);
					getPrefixVector<Key>(prefix, count, 0);
					getUpdatedVector(v, k, tmp, tmpKey, prefix, curShift, 0, SIZE);

					std::swap(v, tmp);
					std::swap(k, tmpKey);

					curShift += SHIFT_BITS;
				}
			}

			template <typename T, typename F>
			requires is_floating_point<invoke_result<T, F>>
			inline void sort_impl(std::vector<T>& v, F func, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				const int SIZE = v.size();
				using U = t2u<Key>;
				constexpr int SIGN_SHIFT = (sizeof(U) * 8) - 1;
				constexpr U SIGN_MASK = 1LL << SIGN_SHIFT;

				std::vector<U> vu(SIZE);

				for (int i = 0; i < SIZE; i++)
				{
					std::memcpy(&vu[i], &func(v[i]), sizeof(U));
					vu[i] = (vu[i] >> SIGN_SHIFT) ? ~vu[i] : vu[i] ^ SIGN_MASK;
				}

				std::vector<int> indices(SIZE);
				std::iota(indices.begin(), indices.end(), 0);

				//sort_impl(v, vu, enableMultiThreading);
				sort_impl(indices, vu, enableMultiThreading);

				sortByIndices(v, indices, enableMultiThreading);

				//for (int i = 0; i < SIZE; i++)
				//	tmp[i] = std::move(v[indices[i]]);

				//std::vector<T> tmp;
				//tmp.reserve(SIZE);
				//for (const auto& i : indices)
				//	tmp.emplace_back(std::move(v[i]));

				//std::swap(tmp, v);

				//for (int i = 0; i < SIZE; i++)
				//{
				//	vu[i] = (vu[i] >> SIGN_SHIFT) ? vu[i] ^ SIGN_MASK : ~vu[i];
				//	std::memcpy(&v[i], &vu[i], sizeof(U));
				//}
			}

			template <typename T, typename F>
			requires (is_large_integral<invoke_result<T, F>> || is_string<invoke_result<T, F>>)
			inline void sort_impl(std::vector<T>& v, F func, bool enableMultiThreading)
			{
				using Key = invoke_result<T, F>;

				int len = getMaxLength(v, func);

				int numOfThreads = std::thread::hardware_concurrency();
				const int LOCAL_BUCKET_THRESHOLD = std::max(static_cast<int>(v.size() / 1000), 1000);
				std::vector<T> tmp(v.size());

				if (!enableMultiThreading || static_cast<int>(v.size() / 256) - (LOCAL_BUCKET_THRESHOLD / 10) < LOCAL_BUCKET_THRESHOLD)
				{
					std::mutex tmpMutex;
					std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

					if constexpr (std::same_as<Key, std::string>)
					{
						std::vector<RegionString> tmpVector;
						sortInstance(v, func, tmp, tmpVector, tmpLock, RegionString(0, v.size(), len, 0), 0);
					}
					else
					{
						std::vector<RegionIntegral> tmpVector;
						sortInstance(v, func, tmp, tmpVector, tmpLock, RegionIntegral(0, v.size(), len), 0);
					}
				}
				else
				{
					std::vector<std::thread> threads;
					std::mutex regionsLock;
					std::mutex idleLock;
					std::atomic<int> runningCounter = numOfThreads;

					if constexpr (std::same_as<Key, std::string>)
					{
						std::vector<RegionString> regions;
						regions.reserve(v.size() / 1000);
						regions.emplace_back(0, v.size(), len, 0);
						for (int i = 0; i < numOfThreads; i++)
						{
							//threads.emplace_back(sortThread<T, RegionString>, std::ref(v), std::ref(tmp), std::ref(regions), std::ref(regionsLock), std::ref(runningCounter), i);
							threads.emplace_back([&v, &func, &tmp, &regions, &regionsLock, &runningCounter, i]() {
								sortThread(v, func, tmp, regions, regionsLock, runningCounter, i);
							});
						}

						for (auto& t : threads)
							t.join();
					}
					else
					{
						std::vector<RegionIntegral> regions;
						regions.reserve(v.size() / 1000);
						regions.emplace_back(0, v.size(), len);
						for (int i = 0; i < numOfThreads; i++)
						{
							//threads.emplace_back(sortThread<T, RegionIntegral>, std::ref(v), std::ref(tmp), std::ref(regions), std::ref(regionsLock), std::ref(runningCounter), i);
							threads.emplace_back([&v, &func, &tmp, &regions, &regionsLock, &runningCounter, i]() {
								sortThread(v, func, tmp, regions, regionsLock, runningCounter, i);
							});
						}

						for (auto& t : threads)
							t.join();
					}
				}
			}

			template <typename T, typename Key>
			requires (is_large_integral<Key> || is_string<Key>)
			inline void sort_impl(std::vector<T>& v, std::vector<Key>& k, bool enableMultiThreading)
			{
				int len = getMaxLength(k);

				int numOfThreads = std::thread::hardware_concurrency();
				const int LOCAL_BUCKET_THRESHOLD = std::max(static_cast<int>(v.size() / 1000), 1000);
				std::vector<T> tmp(v.size());
				std::vector<Key> tmpKey(v.size());

				if (!enableMultiThreading || static_cast<int>(v.size() / 256) - (LOCAL_BUCKET_THRESHOLD / 10) < LOCAL_BUCKET_THRESHOLD)
				{
					std::mutex tmpMutex;
					std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

					if constexpr (std::same_as<Key, std::string>)
					{
						std::vector<RegionString> tmpVector;
						sortInstance(v, k, tmp, tmpKey, tmpVector, tmpLock, RegionString(0, v.size(), len, 0), 0);
					}
					else
					{
						std::vector<RegionIntegral> tmpVector;
						sortInstance(v, k, tmp, tmpKey, tmpVector, tmpLock, RegionIntegral(0, v.size(), len), 0);
					}
				}
				else
				{
					std::vector<std::thread> threads;
					std::mutex regionsLock;
					std::mutex idleLock;
					std::atomic<int> runningCounter = numOfThreads;

					if constexpr (std::same_as<Key, std::string>)
					{
						std::vector<RegionString> regions;
						regions.reserve(v.size() / 1000);
						regions.emplace_back(0, v.size(), len, 0);
						for (int i = 0; i < numOfThreads; i++)
						{
							//threads.emplace_back(sortThread<T, RegionString>, std::ref(v), std::ref(tmp), std::ref(regions), std::ref(regionsLock), std::ref(runningCounter), i);
							threads.emplace_back([&v, &k , &tmp, &tmpKey, &regions, &regionsLock, &runningCounter, i]() {
								sortThread(v, k, tmp, tmpKey, regions, regionsLock, runningCounter, i);
							});
						}

						for (auto& t : threads)
							t.join();
					}
					else
					{
						std::vector<RegionIntegral> regions;
						regions.reserve(v.size() / 1000);
						regions.emplace_back(0, v.size(), len);
						for (int i = 0; i < numOfThreads; i++)
						{
							//threads.emplace_back(sortThread<T, RegionIntegral>, std::ref(v), std::ref(tmp), std::ref(regions), std::ref(regionsLock), std::ref(runningCounter), i);
							threads.emplace_back([&v, &k, &tmp, &tmpKey, &regions, &regionsLock, &runningCounter, i]() {
								sortThread(v, k, tmp, tmpKey, regions, regionsLock, runningCounter, i);
							});
						}

						for (auto& t : threads)
							t.join();
					}
				}
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

				constexpr int INSERTION_SORT_THRESHOLD = (is_string<Key>) ? 10 : 100;
				if (v.size() <= INSERTION_SORT_THRESHOLD)
				{
					insertionSort(v, func, 0, v.size());
					return;
				}

				sort_impl(v, func, enableMultiThreading);
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

	template <typename T, typename F>
	inline void sort(std::vector<T>& v, F func, bool enableMultiThreading = false)
	{
		internal::key::sort_dispatcher(v, func, enableMultiThreading);
	}
};
