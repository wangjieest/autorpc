#include <string>
#include <map>
#include <tuple>
#include <functional>
namespace autorpc
{
    namespace details
    {
        template<size_t...> struct index_sequence {};

        template<size_t N, size_t... Is>
        struct make_index_sequence : make_index_sequence<N - 1, N - 1, Is...> {};

        template<size_t... Is>
        struct make_index_sequence<0, Is...> : index_sequence<Is...>{};

        template<typename T>
        class is_callable
        {
            using yes_type = char[1];
            using no_type  = char[2];

            template <typename Q>
            static yes_type& check(decltype(&Q::operator()));

            template <typename Q>
            static no_type& check(...);
        public:
            enum { value = sizeof(check<T>(nullptr)) == sizeof(yes_type) };
        };

        template<typename ...Args>
        struct is_last_type_callable
        {
            using type = typename std::tuple_element<sizeof...(Args)-1, std::tuple<Args...>>::type;
            enum { value = is_callable<type>::value };
        };

        template<typename T, typename ...Args>
        struct is_last_type_same
        {
            using type = typename std::tuple_element<sizeof...(Args)-1, std::tuple<Args...>>::type;
            enum { value = std::is_same<typename std::decay<type>::type, T>::value };
        };

        template <class F, class Tuple, size_t... I>
        void apply_impl(F&& f, Tuple&& t, index_sequence<I...>)
        {
            f(std::get<I>(std::forward<Tuple>(t))...);
        }

        template <size_t N, class F, class Tuple>
        void apply(F&& f, Tuple&& t)
        {
            apply_impl(std::forward<F>(f), std::forward<Tuple>(t), make_index_sequence<N>{});
        }


        template<typename InputArchive>
        struct Callable
        {
            using fn_eval = void(*)(int id, void* data, InputArchive&);
            using fn_free = void(*)(void*);

            void*   data = nullptr;
            fn_eval evalfunc = nullptr;
            fn_free freefunc = nullptr;
        };

    }


    // 采用模板隔离序列化和反序列化的实现
    // OutputArchive 需要实现返回 std::string 的 encode 函数.
    // InputArchive  需要实现decode函数 + 构造函数
    template<typename OutputArchive, typename InputArchive>
    class Endpoint
    {
    public:
        Endpoint()
        {
        }
        Endpoint(const Endpoint&) = delete;
        Endpoint& operator =(const Endpoint&) = delete;
        Endpoint(Endpoint&&other)
            : seq_(other.seq_)
            , reg_cb_(std::move(other.reg_cb_))
            , reg_rpc_(std::move(other.reg_rpc_))
        {
        }
        Endpoint& operator =(Endpoint&&other)
        {
            seq_ = other.seq_;
            reg_cb_ = std::move(other.reg_cb_);
            reg_rpc_ = std::move(other.reg_rpc_);
        }
        ~Endpoint()
        {
            for (auto&a : reg_cb_)
            {
                auto & cell = a.second;
                cell.freefunc(cell.data);
            }
            for (auto&a : reg_rpc_)
            {
                auto & cell = a.second;
                cell.freefunc(cell.data);
            }
        }

        struct RequestInfo
        {
            Endpoint* endpoint_ = nullptr;
            int seq_ = -1;
        };


        template<typename Func>
        void reg(const char* funname, Func func)
        {
            insertCallable(reg_rpc_, funname, func);
        }

        template<typename ...Args>
        std::string request(const char* funname, Args&& ... args)
        {
            int id = details::is_last_type_callable<Args...>::value ? nextSequenceId() : -1;
            return argsWrite(typename std::conditional<details::is_last_type_callable<Args...>::value, std::true_type, std::false_type>::type{}, funname, id, std::forward<Args>(args)...);
        }

        template<typename ...Args>
        std::string response(int seq, Args&& ...args)
        {
            return argsWrite(std::false_type{}, RPC_RESPONE_METHOD, seq, std::forward<Args>(args)...);
        }

        void process(const char*buf, size_t len)
        {
            eval(buf, len);
        }

    private:
        const char* RPC_RESPONE_METHOD = "rpc_respone";
        using FunctorMap = std::map<std::string, details::Callable<InputArchive>>;

        void eval(const char*buf, size_t len)
        {
            InputArchive ar(buf, len);
            std::string method;
            int id;

            ar.decode(method);
            ar.decode(id);

            if (method == RPC_RESPONE_METHOD)
            {
                auto it = reg_cb_.find(std::to_string(id));
                if (it != reg_cb_.end())
                {
                    auto& cb = it->second;
                    cb.evalfunc(id, cb.data, ar);

                    cb.freefunc(cb.data);
                    reg_cb_.erase(it);
                }
            }
            else
            {
                auto it = reg_rpc_.find(method);
                if (it != reg_rpc_.end())
                {
                    auto& cb = it->second;
                    cb.evalfunc(id, cb.data, ar);
                }
                else
                {
                    //no reg
                }
            }

        }

        template<typename ...Args>
        class VariadicArgFunctor
        {
            using store_type = std::tuple<typename std::remove_const<typename std::remove_reference<Args>::type>::type...>;
            store_type  store_;
            std::function<void(Args...)>   func_;
            Endpoint* endpoint_ = nullptr;

            template<typename Tup, size_t ...Is>
            static void fillargs_impl(InputArchive& ar, Tup&& tup, const details::index_sequence<Is...>&)
            {
                ar.decode(std::get<Is>(std::forward<Tup>(tup))...);
            }

            void fillargs(int, InputArchive& ar, const std::false_type&)
            {
                fillargs_impl(ar, this->store_, details::make_index_sequence<sizeof...(Args)>{});
            }

            void fillargs(int id, InputArchive& ar, const std::true_type&)
            {
                auto &last = std::get<sizeof...(Args)-1>(this->store_);
                last.seq_ = id;
                last.endpoint_ = endpoint_;
                fillargs_impl(ar, this->store_, details::make_index_sequence<sizeof...(Args)-1>{});
            }
        public:
            VariadicArgFunctor(Endpoint* ep, std::function<void(Args...)>&& f)
                : func_(std::move(f))
                , endpoint_(ep)
            {

            }
            static void invoke(int id, void* pvoid, InputArchive& ar)
            {
                auto pThis = static_cast<VariadicArgFunctor<Args...>*>(pvoid);
                pThis->fillargs(id, ar,
                                typename std::conditional<details::is_last_type_same<RequestInfo, Args...>::value, std::true_type, std::false_type>::type{});

                details::apply<sizeof...(Args)>(pThis->func_, pThis->store_);
            }
            static void destroy(void* p)
            {
                auto pThis = static_cast<VariadicArgFunctor<Args...>*>(p);
                delete pThis;
            }

        };


        template<typename ...Args>
        void insertCallable(FunctorMap& callback, std::string name, void(*func)(Args...))
        {
            insertFreeFunction(callback, std::move(name), func);
        }

        template<typename Func>
        void insertCallable(FunctorMap& callback, std::string name, Func& functor)
        {
            insertFunctor(callback, std::move(name), std::move(functor), &Func::operator());
        }

        template<typename ... Args>
        void insertFreeFunction(FunctorMap& callback, std::string&& name, void(*func)(Args...))
        {
            void*data = new VariadicArgFunctor<Args...>(this, func);
            details::Callable<InputArchive> callable;
            callable.data = data;
            callable.evalfunc = static_cast<typename details::Callable<InputArchive>::fn_eval>(&VariadicArgFunctor<Args...>::invoke);
            callable.freefunc = static_cast<typename details::Callable<InputArchive>::fn_free>(&VariadicArgFunctor<Args...>::destroy);
            callback.emplace(std::move(name), callable);
        }

        template<typename Func, typename ...Args>
        void insertFunctor(FunctorMap& callback, std::string&& name, Func&& functor, void(Func::*)(Args...) const)
        {
            void*data = new VariadicArgFunctor<Args...>(this, std::move(functor));
            details::Callable<InputArchive> callable;
            callable.data = data;
            callable.evalfunc = static_cast<typename details::Callable<InputArchive>::fn_eval>(&VariadicArgFunctor<Args...>::invoke);
            callable.freefunc = static_cast<typename details::Callable<InputArchive>::fn_free>(&VariadicArgFunctor<Args...>::destroy);
            callback.emplace(std::move(name), callable);
        }

        int nextSequenceId() { return ++seq_ & (~(1 << (sizeof(decltype(seq_)) * 8 - 1))); }
        int                     seq_{ 0 };
        FunctorMap              reg_cb_;
        FunctorMap              reg_rpc_;



        template<typename ...Args>
        std::string argsWrite(std::false_type, const char* name, int id, Args&&...args)
        {
            OutputArchive ar;
            return ar.encode(name, id, std::forward<Args>(args)...);
        }

        template<typename Tup, size_t ...Is>
        static std::string invoke_impl(const char* name, int id, Tup&& tup, const details::index_sequence<Is...>&)
        {
            OutputArchive ar;
            return ar.encode(name, id, std::get<Is>(std::forward<Tup>(tup))...);
        }

        template<typename ...Args>
        std::string argsWrite(std::true_type, const char* name, int id, Args&&...args)
        {
            auto tup = std::make_tuple(std::forward<Args>(args)...);
            std::string ret = invoke_impl(name, id, tup, details::make_index_sequence<sizeof...(Args)-1>{});
            insertCallable(reg_cb_, std::to_string(id), std::get<sizeof...(Args)-1>(tup));
            return ret;
        }

    };

    //toy classes 
    class InputArchiveSimple
    {
    public:
        InputArchiveSimple& operator =(const InputArchiveSimple&rhs)
        {
            memcpy(this, &rhs, sizeof(*this));
            return *this;
        }
        InputArchiveSimple(const char* buf, size_t len)
            : buf_(const_cast<char*>(buf))
            , cur_(const_cast<char*>(buf))
            , len_(len)
        {
        }

        void decode(){}
        template<typename T, typename ...Args>
        typename std::enable_if<std::is_pod<typename std::decay<T>::type>::value, void>::type decode(T&t, Args&& ... args)
        {
            t = *((T*)cur_);
            cur_ += sizeof(T);
            decode(std::forward<Args>(args)...);
        }
        template<typename T, typename ...Args>
        typename std::enable_if<std::is_same<std::string, typename std::decay<T>::type>::value, void>::type decode(T&&t, Args&& ... args)
        {
            size_t  n;
            decode(n);
            t.assign(cur_, n);
            cur_ += n;
            decode(std::forward<Args>(args)...);
        }
    private:
        char* buf_ = nullptr;
        char* cur_ = nullptr;
        const size_t len_ = 0;
    };


    //output 需要 encode 函数
    class OutputArchiveSimple
    {
    public:
        template<typename ...Args>
        std::string encode(const char* method, int id, Args&& ...args)
        {
            buf_.clear();
            encode_inner(std::string(method), id, std::forward<Args>(args)...);
            return buf_;
        }

    private:
        void encode_inner(){}
        template<typename T, typename ...Args>
        typename std::enable_if<std::is_pod<typename std::decay<T>::type>::value, void>::type encode_inner(T&& t, Args&& ...args)
        {
            buf_.append((const char*)&t, sizeof(T));
            encode_inner(std::forward<Args>(args)...);
        }

        template<typename T, typename ...Args>
        typename std::enable_if<std::is_same<std::string, typename std::decay<T>::type>::value, void>::type encode_inner(T&& t, Args&& ...args)
        {
            size_t n = t.size();
            buf_.append((const char*)&n, sizeof(n));
            buf_.append(t);
            encode_inner(std::forward<Args>(args)...);
        }
        std::string buf_;
    };
}


