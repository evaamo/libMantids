#ifndef ATOMICEXPRESSIONSIDE_H
#define ATOMICEXPRESSIONSIDE_H

#include <boost/regex.hpp>
#include <vector>
#include <string>
#include <set>
#include <mdz_hlp_functions/json.h>


namespace Mantids { namespace Scripts { namespace Expressions {

enum eExpressionSideMode
{
    EXPR_MODE_NUMERIC,
    EXPR_MODE_STATIC_STRING,
    EXPR_MODE_JSONPATH,
    EXPR_MODE_NULL,
    EXPR_MODE_UNDEFINED
};

class AtomicExpressionSide
{
public:
    AtomicExpressionSide(std::vector<std::string> * staticTexts);
    ~AtomicExpressionSide();

    bool calcMode();
    std::string getExpr() const;
    void setExpr(const std::string &value);

    std::set<std::string> resolve(const json & v, bool resolveRegex, bool ignoreCase);

    boost::regex *getRegexp() const;
    void setRegexp(boost::regex *value);

    eExpressionSideMode getMode() const;

private:
    std::set<std::string> recompileRegex(const std::string & r, bool ignoreCase);

    boost::regex * regexp;
    std::vector<std::string> * staticTexts;
    uint32_t staticIndex;
    std::string expr;
    eExpressionSideMode mode;
};
}}}
#endif // ATOMICEXPRESSIONSIDE_H
