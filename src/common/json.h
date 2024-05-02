#ifndef BDD_BENCHMARK_COMMON_JSON_H
#define BDD_BENCHMARK_COMMON_JSON_H

#include <iomanip>
#include <ostream>
#include <string>
#include <type_traits>

namespace json
{
  /// \brief Endline character (without flushing)
  inline std::ostream&
  endl(std::ostream& os)
  {
    return os << '\n';
  }

  /// \brief Flush output stream
  inline std::ostream&
  flush(std::ostream& os)
  {
    return os << std::flush;
  }

  /// \brief Add a comma
  inline std::ostream&
  comma(std::ostream& os)
  {
    return os << ',';
  }

  /// \brief 'null' value
  inline std::ostream&
  nil(std::ostream& os)
  {
    return os << "null";
  }

  /// \brief Indentation level.
  extern int indent_level;

  /// \brief Add indentation space
  inline std::ostream&
  indent(std::ostream& os)
  {
    return os << std::left << std::setw(2 * indent_level) << "";
  }

  /// \brief Open brace for a new (sub)struct.
  ///
  /// \remark Indentation for objects inside of arrays is not taken care of.
  inline std::ostream&
  brace_open(std::ostream& os)
  {
    indent_level++;
    return os << "{";
  }

  /// \brief Close brace for a (sub)struct.
  inline std::ostream&
  brace_close(std::ostream& os)
  {
    indent_level--;
    return os << indent << "}";
  }

  /// \brief Open brace for a new (sub)array.
  ///
  /// \remark Indentation for arrays inside of arrays is not taken care of.
  inline std::ostream&
  array_open(std::ostream& os)
  {
    indent_level++;
    return os << "[";
  }

  /// \brief Close brace for a (sub)array.
  inline std::ostream&
  array_close(std::ostream& os)
  {
    indent_level--;
    return os << indent << "]";
  }

  /// \brief Create a new field with a given name.
  struct field
  {
  private:
    const std::string _x;

  public:
    field(const std::string x)
      : _x(x)
    {}

    template <class Elem, class Traits>
    friend std::basic_ostream<Elem, Traits>&
    operator<<(std::basic_ostream<Elem, Traits>& os, const field& f)
    {
      return os << indent << "\"" << f._x << "\": ";
    }
  };

  /// \brief Output a single value.
  template <typename T>
  struct value
  {
  private:
    const T _t;

  public:
    value(T t)
      : _t(t)
    {}

    template <class Elem, class Traits>
    friend std::basic_ostream<Elem, Traits>&
    operator<<(std::basic_ostream<Elem, Traits>& os, const value& v)
    {
      if constexpr (std::is_same<T, std::string>::value
                    || std::is_same<T, std::string_view>::value) {
        return os << "\"" << v._t << "\"";
      } else if constexpr (std::is_same<T, bool>::value) {
        return os << (v._t ? "true" : "false");
      } else {
        return os << v._t;
      }
    }
  };
}

#endif // BDD_BENCHMARK_COMMON_JSON_H
