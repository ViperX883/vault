#include <atomic>
#include <utility>
#include <type_traits>

template<typename T, std::size_t N, typename = std::enable_if_t<(N > 0)>>
class hybrid_allocator_t {
    using data_t = std::aligned_storage_t<sizeof(T), alignof(T)>;
    
    union node_t {
        node_t *m_next;
        data_t  m_data;
    };

    using head_t = std::atomic<node_t *>;
    
    mutable node_t m_storage[10];
    mutable head_t m_head;

public:
    constexpr hybrid_allocator_t() noexcept
      : m_storage(), m_head(m_storage)
    {
        for(std::size_t i = 0; i < N-1; ++i) {
	  m_storage[i].m_next = m_storage + i + 1;
        }
    }
    
    void *malloc() const { // can't be noexcept - new can throw
        auto *head = m_head.load(std::memory_order_relaxed);

        // If `head` is `NULL` then there is no storage left, so use normal `malloc`.
        if(head == nullptr) {
            return ::operator new(sizeof(T));
        }
        
        // Otherwise we want to swap `head` with `head->next`.  If this succeeds
        // then we own the storage pointed to by `head` and we can return it.
        // Otherwise we've been preempted by another thread and we fall back to
        // normal `malloc`, just as in the case above.
        if(!m_head.compare_exchange_strong(head, head -> m_next, std::memory_order_release, std::memory_order_relaxed)) {
            return ::operator new(sizeof(T));
        }

	// We're done.
	return head;
    }
    
    template<typename... Args>
    T *construct(Args&&... p_args) const {
        return new (this -> malloc()) T (std::forward<Args>(p_args)...);
    }
    
    void free(void *p_ptr) const noexcept {

        // First check if the pointer references an element in our static slab.
        // If not it came from the free store and we call the normal `delete`.
        if(p_ptr < m_storage || p_ptr >= (m_storage + N)) {
            ::operator delete(p_ptr);  return;
        }

	// Since the node is from our static slab we need to put it back
	// in the free list.  To do this we atomically swap our node into
	// the head position.        
	auto *new_head = static_cast<node_t *>(p_ptr);
        new_head -> m_next = m_head.load(std::memory_order_relaxed);
        
        // Unlike the allocation case where we could bail out if accessing the static
        // storage failed, here we MUST return the node to the free list.  We also need
        // to make sure `new_head->m_next` is properly initialized BEFORE inserting
        // it into the free list, so we need to reinitialize it in each iteration of the
        // loop.
        while(!m_head.compare_exchange_weak(new_head -> m_next, new_head, std::memory_order_release, std::memory_order_relaxed));
    }
    
    void destroy(T *p_ptr) const noexcept(noexcept(p_ptr -> ~T())) {
        p_ptr -> ~T();  this -> free(p_ptr);
    }
    
}; // end struct hybrid_allocator_t
