#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <memory>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    
    RawMemory(const RawMemory& other) = delete;
    
    
    RawMemory& operator = (const RawMemory& rhs) = delete;
    
    
    //После перемещения новый вектор станет владеть данными исходного вектора. Исходный вектор будет иметь нулевой размер и вместимость и ссылаться на nullptr.
    RawMemory(RawMemory&& other) noexcept {
        if (this != &other) {
            buffer_ = std::exchange(other.buffer_, nullptr);
            capacity_ = std::exchange(other.capacity_, 0);
            Deallocate(other.buffer_);
        }   
    }
    
    
    RawMemory& operator = (RawMemory&& rhs) noexcept {
        buffer_ = std::move(rhs.buffer_);
        capacity_ = std::move(rhs.capacity_);
        return *this;
    }
    
    
    ~RawMemory() {
        Deallocate(buffer_);
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
    // dealocate allocated memory
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
    
    //Конструктор по умолчанию. Инициализирует вектор нулевого размера и вместимости. Не выбрасывает исключений. Алгоритмическая сложность: O(1).
    Vector() = default;
    
   //Конструктор, который создаёт вектор заданного размера. Вместимость созданного вектора равна его размеру, а элементы проинициализированы значением по умолчанию для типа T. Алгоритмическая сложность: O(размер вектора). 
   Vector(size_t size)
    :data_(size) // can throw exception
    ,size_(size) {
        
    std::uninitialized_value_construct_n(data_.GetAddress(), size_);     
    }    
    
    
    //Копирующий конструктор. Создаёт копию элементов исходного вектора. Имеет вместимость, равную размеру исходного вектора, то есть выделяет память без запаса. Алгоритмическая сложность: O(размер исходного вектора)
    Vector(const Vector& other)
        :data_(other.size_) // can throw exception
        ,size_(other.size_){
            
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());    
    }
    
    
    //move constructor 
    Vector(Vector&& other) {
        if (this != &other) {
            Swap(other);
            other.size_ = 0;
        }  
    }
   

    Vector<T>& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector<T> rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + std::min(size_, rhs.size_), data_.GetAddress());

                if (size_ < rhs.size_) {
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                } else {
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }


    Vector<T>& operator =(Vector&& rhs) {
        if (this != &rhs) {
           Swap(rhs);
        }
        return *this;
    }

    
    void Resize (size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_ + new_size, size_ - new_size);
        } else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
        }
        size_ = new_size;
    }


    void PopBack() noexcept {
        std::destroy_at(data_ + (size_ - 1));
        --size_;
    }


    void PushBack(const T& val) {
       EmplaceBack(std::forward<const T&>(val)); 
    }


    void PushBack(T&& val) {
        EmplaceBack(std::forward<T&&>(val));
    }


  //Метод void Reserve(size_t capacity). Резервирует достаточно места, чтобы вместить количество элементов, равное capacity. Если новая вместимость не превышает текущую, метод не делает ничего. Алгоритмическая сложность: O(размер вектора).    
    void Reserve(size_t new_capacity){
        if (new_capacity <= data_.Capacity()){
            return;
        }
        RawMemory<T> new_data (new_capacity);

         // Конструируем элементы в new_data, копируя их из data_
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {

             std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }

        // Разрушаем элементы в data_
        std::destroy_n(data_.GetAddress(), size_);

        // Избавляемся от старой сырой памяти, обменивая её на новую
        data_.Swap(new_data);
        // При выходе из метода старая память будет возвращена в кучу
    }
    
    
    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }


    iterator Insert (const_iterator pos, const T& val){
        return Emplace(pos, val);
    }

    iterator Insert (const_iterator pos, T&& val){
        return Emplace(pos, std::move(val));
    }


    iterator Erase (const_iterator pos) {
        assert(pos >= begin() && pos <= end());
        size_t shift = pos - begin();
        std::move(data_ + shift + 1, end(), data_ + shift);
        PopBack();
        return begin() + shift;
    }
    
    
    size_t Size() const noexcept {
        return size_;
    }

    
    size_t Capacity() const noexcept {
        return data_.Capacity();
    }
    
  
    template <typename...Args>
    iterator EmplaceWithRealocation (const_iterator pos, Args&&... args) {
        iterator iter = nullptr;
        size_t shift = pos - begin();
        RawMemory<T> new_data (size_ == 0 ? 1 : 2*size_);
            iter = new(new_data + shift) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), shift, new_data.GetAddress());
                std::uninitialized_move_n(data_ + shift, size_ - shift, new_data + shift + 1);
            } else {
                    std::uninitialized_copy_n(data_.GetAddress(), shift, new_data.GetAddress());
                    std::uninitialized_copy_n(data_ + shift, size_ - shift, new_data + shift + 1);
                } 
   
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        return iter;
    }
    
    
    template <typename...Args>
    iterator EmplaceWithoutRealocation (const_iterator pos, Args&&... args) {
        iterator iter = nullptr;
        auto index = static_cast<size_t>(pos - begin());
         
            if (index == size_) {
                new(data_ + size_) T(std::forward<Args>(args)...);
            } else {
                T copy(std::forward<Args>(args)...);
                new(data_ + size_) T(std::move(data_[size_ - 1]));
             
                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    std::move_backward(begin() + index, end() - 1, end());
                    data_[index] = std::move(copy);  
                } else {
                    try {
                        std::copy_backward(begin() + index, end() - 1, end());
                         data_[index] = std::move(copy);
                    } catch (...) {
                        std::destroy_n(end(), 1);
                        //std::destroy_n(data_.GetAddress() + size_, 1);
                        throw;
                    }  
                }
            }
             iter = begin() + index;
        return iter;
    }

    
    template <typename...Args>
    iterator Emplace (const_iterator pos, Args&&... args) {
        iterator iter = nullptr;
        if (size_ < data_.Capacity()) {
            iter = EmplaceWithoutRealocation (pos, std::forward<Args>(args)...);
        }else {
            iter = EmplaceWithRealocation(pos, std::forward<Args>(args)...);
        }
        ++size_;
        return iter;
    }
    
    
    template <typename...Args>
    T& EmplaceBack(Args&&...args) {
         iterator it = Emplace(cend(), std::forward<Args>(args)...);
        return *it;
    }
    
   
     const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }
    

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }
    
    
      //Деструктор. Разрушает содержащиеся в векторе элементы и освобождает занимаемую ими память. Алгоритмическая сложность: O(размер вектора).   
~Vector(){
    std::destroy_n(data_.GetAddress(), size_);
}

    
iterator begin() noexcept {
    return data_.GetAddress();
}

    
iterator end() noexcept{
    return data_ + size_;
}

    
const_iterator begin() const noexcept{
    return const_iterator(data_.GetAddress());
}

    
const_iterator end() const noexcept{
    return const_iterator(data_ + size_);
}


const_iterator cbegin() const noexcept {
    return const_iterator(data_.GetAddress());
}

    
const_iterator cend() const noexcept {
    return const_iterator(data_ + size_);
}
   
private:
    RawMemory<T> data_;
    size_t size_ = 0;
};


