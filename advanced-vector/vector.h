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
    
    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;
    
    //Конструктор по умолчанию. Инициализирует вектор нулевого размера и вместимости. Не выбрасывает исключений. Алгоритмическая сложность: O(1).
    Vector() = default;
    
   //Конструктор, который создаёт вектор заданного размера. Вместимость созданного вектора равна его размеру, а элементы проинициализированы значением по умолчанию для типа T. Алгоритмическая сложность: O(размер вектора). 
    Vector (size_t size);
  
    //Копирующий конструктор. Создаёт копию элементов исходного вектора. Имеет вместимость, равную размеру исходного вектора, то есть выделяет память без запаса. Алгоритмическая сложность: O(размер исходного вектора).
    Vector(const Vector& other);
    
    //move constructor 
    Vector (Vector&& other);
   
    Vector& operator=(const Vector& rhs);
    
    Vector& operator=(Vector&& rhs);
    
   //Метод void Reserve(size_t capacity). Резервирует достаточно места, чтобы вместить количество элементов, равное capacity. Если новая вместимость не превышает текущую, метод не делает ничего. Алгоритмическая сложность: O(размер вектора).
    void Reserve (size_t capacity);
    
    void Resize (size_t size);
    
    void PopBack() noexcept;
    
    void PushBack(const T& val);
    
    void PushBack(T&& val);

    template <typename...Args>
    T& EmplaceBack(Args&&...args) {
        T* obj = nullptr;
        if (size_ < data_.Capacity()) {
            obj = new(data_ + size_) T(std::forward<Args>(args)...);
        } else {
            RawMemory<T> new_data(size_ == 0 ? 1 : 2*size_);
            obj = new(new_data + size_) T(std::forward<Args>(args)...);
            if constexpr(std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_); 
            data_.Swap(new_data);
        }
        ++size_;
        return *obj;
    }
    
    
   template <typename U>
void PushBackImpl (U&& val) {
    if (size_ == data_.Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<U>(val));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                try {
                    std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
                }
                catch (...) {
                    std::destroy_n(new_data.GetAddress() + size_, 1);
                    throw;
                }
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            new (data_ + size_) T(std::forward<U>(val));
        }
        ++size_;
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
    
    void Swap (Vector& ) noexcept;
   


    template <typename...Args>
    iterator Emplace (const_iterator pos, Args&&... args) {
        assert(pos >= begin() && pos<= end());
        iterator iter = nullptr;
        size_t shift = pos - begin();
        if (size_ < data_.Capacity()) {
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
                        std::destroy(end(), 1);
                        throw;
                    }
                    
                }
               
            }
            
             iter = begin() + index;

            


            }  else {

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
            }
            
            
            
           
      
        
        ++size_;
        return iter;
        }
    

    iterator Insert (const_iterator pos, const T& val);
    
    iterator Insert (const_iterator pos, T&& val);
    
    iterator Erase (const_iterator pos);
      //Деструктор. Разрушает содержащиеся в векторе элементы и освобождает занимаемую ими память. Алгоритмическая сложность: O(размер вектора).
    ~Vector();
    
    
    

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};


template <typename T>
Vector<T>::Vector(size_t size)
    :data_(size) // can throw exception
    ,size_(size) {
     
    std::uninitialized_value_construct_n(data_.GetAddress(), size_);      
    }    


template <typename T> 
Vector<T>::Vector(const Vector& other)
        :data_(other.size_) // can throw exception
        ,size_(other.size_){
            
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());    
    }


template <typename T>
Vector<T>::Vector(Vector&& other) {
    if (this != &other) {
        Swap(other);
        other.size_ = 0;
    }  
}


template <typename T>
Vector<T>& Vector<T>::operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (data_.Capacity() < rhs.size_) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                if (rhs.size_ < size_) {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                else {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                }
                size_ = rhs.size_;
 
            }
        }
        return *this;
    }


template <typename T>
Vector<T>& Vector<T>::operator =(Vector&& rhs) {
    if (this != &rhs) {
       Swap(rhs);
    }
    return *this;
}


template <typename T>
void Vector<T>::Resize (size_t new_size) {
    if (new_size < size_) {
        std::destroy_n(data_ + new_size, size_ - new_size);
    } else {
        Reserve(new_size);
        std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
    }
    size_ = new_size;
}

template <typename T>
void Vector<T>::PopBack() noexcept {
    
    std::destroy_at(data_ + (size_ - 1));
    --size_;
}



template <typename T>
void Vector<T>::PushBack(const T& val) {
  PushBackImpl(val); 
}


template <typename T>
void Vector<T>::PushBack(T&& val) {
   Vector<T>::PushBackImpl(std::move(val));
}


template <typename T>
void Vector<T>::Reserve(size_t new_capacity){
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


template <typename T>
void Vector<T>::Swap(Vector& other) noexcept {
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
}


template <typename T>
Vector<T>::iterator Vector<T>::Insert (const_iterator pos, const T& val){
    return Emplace(pos, val);
}

template <typename T>
Vector<T>::iterator Vector<T>::Insert (Vector<T>::const_iterator pos, T&& val){
    return Emplace(pos, std::move(val));
}


template <typename T>
Vector<T>::iterator Vector<T>::Erase (Vector<T>::const_iterator pos) {
    assert(pos >= begin() && pos <= end());
  
    size_t shift = pos - begin();
    std::move(data_ + shift + 1, end(), data_ + shift);
    PopBack();
   
    return begin() + shift;
}


template <typename T>
Vector<T>::~Vector(){
    std::destroy_n(data_.GetAddress(), size_);
  
}

template <typename T>
Vector<T>::iterator Vector<T>::begin() noexcept {
    return data_.GetAddress();
}

template <typename T>
Vector<T>::iterator Vector<T>::end() noexcept{
    return data_ + size_;
}

template <typename T>
Vector<T>::const_iterator Vector<T>::begin() const noexcept{
    return const_iterator(data_.GetAddress());
}

template <typename T>
Vector<T>::const_iterator Vector<T>::end() const noexcept{
    return const_iterator(data_ + size_);
}


template <typename T>
Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
    return const_iterator(data_.GetAddress());
}

template <typename T>
Vector<T>::const_iterator Vector<T>::cend() const noexcept {
    return const_iterator(data_ + size_);
}


