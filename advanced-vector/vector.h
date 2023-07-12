#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:
	RawMemory() = default;

	explicit RawMemory(size_t capacity)
		: buffer_(Allocate(capacity))
		, capacity_(capacity) {
	}

	~RawMemory() {
		Deallocate(buffer_);
	}

	RawMemory(const RawMemory&) = delete;
	RawMemory& operator=(const RawMemory& rhs) = delete;
	RawMemory(RawMemory&& other) noexcept : buffer_(std::move(other.buffer_)), capacity_(std::move(other.capacity_)) {
		other.buffer_ = nullptr;
		other.capacity_ = 0;
	}
	RawMemory& operator=(RawMemory&& rhs) noexcept {
		if (this == &rhs) {
			return *this;
		}
		~RawMemory();
		buffer_ = std::move(rhs.buffer_);
		capacity_ = std::move(rhs.capacity_);
		rhs.buffer_ = nullptr;
		rhs.capacity_ = 0;
	}

	T* operator+(size_t offset) noexcept {
		// Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
		assert(offset <= capacity_);
		return buffer_ + offset;
	}

	const T* operator+(size_t offset) const noexcept {
		return const_cast<RawMemory&>(*this) + offset;
	}

	const T& operator[](size_t index) const noexcept {
		return const_cast<RawMemory&>(*this)[index];
	}

	T& operator[](size_t index) noexcept {
		assert(index < capacity_);
		return buffer_[index];
	}

	void Swap(RawMemory& other) noexcept {
		std::swap(buffer_, other.buffer_);
		std::swap(capacity_, other.capacity_);
	}

	const T* GetAddress() const noexcept {
		return buffer_;
	}

	T* GetAddress() noexcept {
		return buffer_;
	}

	size_t Capacity() const {
		return capacity_;
	}

private:
	// Выделяет сырую память под n элементов и возвращает указатель на неё
	static T* Allocate(size_t n) {
		return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
	}

	// Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
	static void Deallocate(T* buf) noexcept {
		operator delete(buf);
	}

	T* buffer_ = nullptr;
	size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
	using iterator = T*;
	using const_iterator = const T*;

	iterator begin() noexcept {
		return data_.GetAddress();
	};
	iterator end() noexcept {
		return data_ + size_;
	};
	const_iterator begin() const noexcept {
		return data_.GetAddress();
	};
	const_iterator end() const noexcept {
		return data_ + size_;
	};
	const_iterator cbegin() const noexcept {
		return begin();
	};
	const_iterator cend() const noexcept {
		return end();
	};

	Vector() = default;

	explicit Vector(size_t size)
		: data_(size)
		, size_(size) {
		std::uninitialized_value_construct_n(data_.GetAddress(), size);
	}

	Vector(const Vector& other)
		: data_(other.size_)
		, size_(other.size_) {
		std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
	}

	Vector(Vector&& other) noexcept
		: data_(std::move(other.data_))
		, size_(std::move(other.size_)) {
		other.size_ = 0;
	};

	Vector& operator=(const Vector& rhs) {
		if (this != &rhs) {
			if (rhs.size_ > data_.Capacity()) {
				Vector rhs_copy(rhs);
				Swap(rhs_copy);
			}
			else {
				if (rhs.size_ < size_) {
					for (size_t i = 0; i != rhs.size_; ++i) {
						data_[i] = rhs.data_[i];
					}
					std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
					size_ = rhs.size_;
				}
				else {
					for (size_t i = 0; i != size_; ++i) {
						data_[i] = rhs.data_[i];
					}
					std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
					size_ = rhs.size_;
				}
			}
		}
		return *this;
	};
	Vector& operator=(Vector&& rhs) noexcept {
		if (this != &rhs) {
			if (rhs.size_ > data_.Capacity()) {
				Vector rhs_copy(std::move(rhs));
				Swap(rhs_copy);
			}
			else {
				if (rhs.size_ < size_) {
					Swap(rhs);
					std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
				}
				else {
					Swap(rhs);
				}
			}
		}
		return *this;
	};

	void Swap(Vector& other) noexcept {
		data_.Swap(other.data_);
		std::swap(size_, other.size_);
	};

	void Reserve(size_t new_capacity) {
		if (new_capacity <= data_.Capacity()) {
			return;
		}
		RawMemory<T> new_data(new_capacity);
		if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
			std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
		}
		else {
			std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
		}
		std::destroy_n(data_.GetAddress(), size_);
		data_.Swap(new_data);
	}

	size_t Size() const noexcept {
		return size_;
	}

	size_t Capacity() const noexcept {
		return data_.Capacity();
	}

	const T& operator[](size_t index) const noexcept {
		return const_cast<Vector&>(*this)[index];
	}

	T& operator[](size_t index) noexcept {
		assert(index < size_);
		return data_[index];
	}

	void Resize(size_t new_size) {
		if (new_size < size_) {
			std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
			size_ = new_size;
		}
		else if (new_size > size_) {
			Reserve(new_size);
			std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
			size_ = new_size;
		}
	};

	void PushBack(const T& value) {
		EmplaceBack(value);
	};

	void PushBack(T&& value) {
		EmplaceBack(std::move(value));
	};

	void PopBack() noexcept {
		std::destroy_n(data_.GetAddress() + size_ - 1, 1);
		--size_;
	};

	template <typename... Args>
	T& EmplaceBack(Args&&... args) {
		T* elem_pointer = nullptr;
		if (size_ == Capacity()) {
			Vector new_vector;
			new_vector.Reserve(size_ == 0 ? 1 : size_ * 2);
			new_vector.size_ = size_;
			elem_pointer = new(new_vector.data_ + size_) T(T(std::forward<Args>(args)...));
			if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
				std::uninitialized_move_n(data_.GetAddress(), size_, new_vector.data_.GetAddress());
			}
			else {
				std::uninitialized_copy_n(data_.GetAddress(), size_, new_vector.data_.GetAddress());
			}
			Swap(new_vector);
		}
		else {
			elem_pointer = new(data_ + size_) T(T(std::forward<Args>(args)...));
		}
		++size_;
		return *elem_pointer;
	}

	template <typename... Args>
	iterator Emplace(const_iterator pos, Args&&... args) {
		T* elem_pointer = nullptr;
		size_t index = pos - data_.GetAddress();
		if (pos == end()) {
			EmplaceBack(std::forward<Args>(args)...);
		}
		else {
			if (size_ == Capacity()) {
				Vector new_vector;
				new_vector.Reserve(size_ == 0 ? 1 : size_ * 2);
				new_vector.size_ = size_;
				elem_pointer = new(new_vector.begin() + index) T(T(std::forward<Args>(args)...));
				if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
					std::uninitialized_move_n(begin(), index, new_vector.begin());
					std::uninitialized_move_n(begin() + index, size_ - index, new_vector.begin() + index + 1);
				}
				else {
					std::uninitialized_copy_n(begin(), index, new_vector.begin());
					std::uninitialized_copy_n(begin() + index, size_ - index, new_vector.begin() + index + 1);
				}
				Swap(new_vector);
			}
			else {
				elem_pointer = new T(T(std::forward<Args>(args)...));
				new(end()) T(std::move(*(end() - 1)));
				//std::destroy_n(end() - 1, 1);
				std::move_backward(begin() + index, end() - 1 , end());
				std::move(elem_pointer, elem_pointer + 1, begin() + index);
				std::destroy_n(elem_pointer, 1);
				//delete elem_pointer;
			}
			++size_;
		}
		return begin() + index;
	}

	iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
		size_t index = pos - data_.GetAddress();
		std::move(begin() + index + 1, end(), begin() + index);
		PopBack();
		return begin() + index;
	};

	iterator Insert(const_iterator pos, const T& value) {
		return Emplace(pos, value);
	};

	iterator Insert(const_iterator pos, T&& value) {
		return Emplace(pos, std::move(value));
	};

	~Vector() {
		std::destroy_n(data_.GetAddress(), size_);
	}

private:
	RawMemory<T> data_;
	size_t size_ = 0;
};