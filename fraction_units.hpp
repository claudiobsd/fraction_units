#pragma once

#include <cmath>
#include <array>
#include <algorithm>

#define _CONSTEVAL_ consteval
#define _CONSTEXPR_ constexpr

enum UnitError {
    NoError=0,
    InvalidToken,
    InvalidExponent,
    BadParenthesis,
    DefinitionTooLong,
    TooManyTokens
};

static _CONSTEXPR_ const char *UnitErrorMessages[]={
    "OK",
    "Invalid Unit Name",
    "Invalid Exponent",
    "Bad Parenthesis open/close count",
    "Unit definition exceeds maximum length, increase maxDefinitionLength",
    "Unit definition exceeds maximum number of tokens, increase maxTokens"

};

// Configuration constants
constexpr size_t maxTokens = 20;
constexpr size_t maxTokenLength = 16;
constexpr size_t maxDefinitionLength = maxTokens*maxTokenLength/2;


// String literals as templates for complex units
template<size_t N>
struct UnitPart {
    constexpr UnitPart(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    char value[N]={};
};

// Basic math because we cannot use library functions within consteval
_CONSTEVAL_ int64_t gcd(int64_t a, int64_t b)
{
    while(b) {
        auto t=b;
        b=a%b;
        a=t;
    }
    return a;
}

_CONSTEVAL_ int64_t intpow(int64_t number,int64_t exp)
{
    int64_t result=number;
    --exp;
    while(exp) {
        if(exp&1) result*=number;
        else number*=number;
        exp>>=1;
    }
    return result;
}

_CONSTEVAL_ int64_t intabs(int64_t number) {
    return (number<0)? -number:number;
}

_CONSTEVAL_ std::pair<int64_t,int64_t> addfraction(int64_t a_num,int64_t a_den, int64_t b_num, int64_t b_den)
{
    std::pair<int64_t,int64_t> result;
    result.first = a_num*b_den+b_num*a_den;
    result.second = a_den*b_den;
    auto divisor = gcd(result.first,result.second);

    result.first/=divisor;
    result.second/=divisor;
    return result;
}

// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************

// Main class containing a unit definition
struct UnitDefinition {

    struct tokendata {
        size_t tokStart=0;
        size_t tokEnd=0;
        int64_t expNum=0;
        int64_t expDen=0;
    };

    // Default constructor creates the non-dimensional number 1.0
    constexpr UnitDefinition() : u_name(""),u_def(""), value_ip(1),value_den(1),value_exp(0), error_state(NoError), error_index(0), definition({}) {}
    // Constructor using a single string literal, creates a unit definition with no name
    template<size_t M>
    constexpr UnitDefinition(const UnitPart<M> U) : UnitDefinition("",U.value) {}
    // Main constructor and parser, creates a unit definition from a name and a string
    template<size_t N, size_t M>
    constexpr UnitDefinition(const char (&name)[N], const char (&defstring)[M]) {
        std::copy_n(name, N, u_name);
        std::copy_n(defstring, M, u_def);
        u_defLen=M;
        bool haveDen = false;
        // Numerical part or a unit is composed of 3 64-bit integers: (numerator/denominator)*10^exponent
        // This provides same of better range than any double precision, while using integer arithmetic during compile time
        int64_t intPart=0;
        int64_t expTen=0;
        int64_t denPart=0;
        int64_t denExpTen=0;
        // Parse the value first
        int expmove = 0;
        size_t i=0;
        while(i<M) {
            if(u_def[i]>='0'&&u_def[i]<='9') {
                intPart*=10;
                intPart+=u_def[i]-'0';
                expTen+=expmove;
            } else {
                if(u_def[i]=='.') {
                    expmove=1;
                } else {
                    if(u_def[i]=='/' && !haveDen && u_def[i+1]>='0' && u_def[i+1]<='9') {
                        // Looks like we have a denominator in the value
                        // Store the current number in the denominator and start over, we'll swap them later
                        denPart=intPart;
                        denExpTen=expTen;
                        intPart=0;
                        expTen=0;
                        haveDen=true;
                    } else {
                    // No other chars allowed in the number
                    // In the future, see if we can also accept exponents here
                    break;
                }
                }
            }
            ++i;
        }
        if(haveDen) {
            // Swap numerator and denominator
            auto temp=intPart;
            intPart=denPart;
            denPart=temp;
            // Subtract the exponents of numerator and denominator, we use only one exponent
            expTen=denExpTen-expTen;
        }
        else {
            denPart=1;
        }
        // If no value is provided, assume 1.0
        if(intPart==0) {
            intPart=1;
            expTen=0;
        }

        // Done with the value, get the sequence of units
        // by consuming tokens
        struct parenthesisdata {
            int firstToken = 0;
            int lastToken = 0;
            int64_t numExp = 0;
            int64_t denExp = 0;
        };

        std::array<tokendata,maxTokens> allTokens;
        std::array<parenthesisdata,50> parenLevel;
        int nTokens=0;
        int nParen=0;

        if(u_def[i]=='_') {
            ++i;
        }
        bool lastTokenWasParen = false;
        size_t tokStart=i;
        size_t tokEnd=i;
        int64_t expnum=1;
        int64_t expden=1;
        while(i<(M-1)) {
            if(u_def[i]=='*' || u_def[i]=='-') {
                // This is a multiplication
                if(lastTokenWasParen) {
                    // Consume the whole parenthesis
                    for(auto j=parenLevel[nParen-1].firstToken;j<parenLevel[nParen-1].lastToken;++j) {
                        // Apply the current exponent to all the items in the parenthesis
                        allTokens[j].expNum *= expnum;
                        allTokens[j].expDen *= expden;
                    }
                    --nParen;
                    lastTokenWasParen=false;
                } else {
                    if(i>tokStart) {
                        tokEnd=i;
                        // Consume the token
                        allTokens[nTokens]={tokStart,tokEnd,expnum,expden};
                        ++nTokens;
                    }
                    tokStart=tokEnd=i+1;
                }
                // Reset next exponent
                expnum=1;
                expden=1;
            }
            else {
                if(u_def[i]==' ') {
                    // This is a multiplication unless it's the beginning of a token
                    if(lastTokenWasParen) {
                    } else {
                        if(i>tokStart) {
                            tokEnd=i;
                            // Consume the token
                            allTokens[nTokens]={tokStart,tokEnd,expnum,expden};
                            ++nTokens;
                            // Reset next exponent
                            expnum=1;
                            expden=1;
                        }
                        tokStart=tokEnd=i+1;
                    }
                }
                else {
                    if(u_def[i]=='/') {
                        if(lastTokenWasParen) {
                            // Consume the whole parenthesis
                            for(auto j=parenLevel[nParen-1].firstToken;j<parenLevel[nParen-1].lastToken;++j) {
                                // Apply the current exponent to all the items in the parenthesis
                                allTokens[j].expNum *= expnum;
                                allTokens[j].expDen *= expden;
                            }
                            --nParen;
                            lastTokenWasParen=false;
                        } else {
                            if(i>tokStart) {
                                tokEnd=i;
                                // Consume the token
                                allTokens[nTokens]={tokStart,tokEnd,expnum,expden};
                                ++nTokens;
                            }
                            tokStart=tokEnd=i+1;
                        }
                        // Reset next exponent
                        expnum=-1;
                        expden=1;
                    }
                    else {
                        if(u_def[i]=='(') {
                            if(i>tokStart) {
                                // We have an open token, close it and multiply
                                tokEnd=i;
                                // Consume the token
                                allTokens[nTokens]={tokStart,tokEnd,expnum,expden};
                                ++nTokens;
                            }
                            tokStart=tokEnd=i+1;
                            parenLevel[nParen].firstToken=nTokens;
                            parenLevel[nParen].lastToken=nTokens;
                            parenLevel[nParen].numExp=expnum;
                            parenLevel[nParen].denExp=expden;
                            ++nParen;
                            expnum=1;
                            expden=1;
                        }
                        else {
                            if(u_def[i]==')') {
                                if(i>tokStart) {
                                    // We have an open token, close it and multiply
                                    tokEnd=i;
                                    // Consume the token
                                    allTokens[nTokens]={tokStart,tokEnd,expnum,expden};
                                    ++nTokens;
                                }
                                tokStart=tokEnd=i+1;
                                // Reset next exponent
                                expnum=1;
                                expden=1;
                                lastTokenWasParen = true;
                                if(nParen>0) {
                                    // Close the parenthesis but don't consume it
                                    parenLevel[nParen-1].lastToken = nTokens;
                                    // Apply the exponent of the parenthesis group to all the tokens
                                    for(auto j=parenLevel[nParen-1].firstToken;j<parenLevel[nParen-1].lastToken;++j) {
                                        // Apply the current exponent to all the items in the parenthesis
                                        allTokens[j].expNum *= parenLevel[nParen-1].numExp;
                                        allTokens[j].expDen *= parenLevel[nParen-1].denExp;
                                    }

                                }
                            }
                            else {
                                if(u_def[i]=='^') {
                                    // Consume a fractional exponent
                                    if(lastTokenWasParen==false) {
                                        if(i>tokStart) {
                                            // We have an open token, close it and multiply
                                            tokEnd=i;
                                            // Consume the token
                                            allTokens[nTokens]={tokStart,tokEnd,expnum,expden};
                                            ++nTokens;
                                        }
                                        tokStart=tokEnd=i+1;
                                    }

                                    ++i;
                                    bool haveNum=false;
                                    bool needDen = false;
                                    bool haveParen=i<(M-1) && u_def[i]=='(';
                                    int64_t num=0;
                                    int64_t den=0;
                                    while(i<(M-1)) {
                                        if(u_def[i]>='0'&&u_def[i]<='9') {
                                            if(!haveNum) {
                                                num*=10;
                                                num+=u_def[i]-'0';
                                            }
                                            else {
                                                den*=10;
                                                den+=u_def[i]-'0';
                                                needDen=false;
                                            }
                                        } else {
                                            if(u_def[i]=='/' && !haveNum) {
                                                haveNum=true;
                                                needDen=true;
                                            } else {
                                                if(u_def[i]==')' && haveParen) {
                                                    ++i;
                                                    break;
                                                } else {
                                                    if(u_def[i]==' ') {

                                                    }
                                                    else {
                                                        // No other chars allowed in the number
                                                        // In the future, see if we can also accept exponents here
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                        ++i;
                                    }

                                    if(den==0) {
                                        den=1;
                                    }

                                    // Apply the exponent to the last token or the last parenthesis group
                                    if(lastTokenWasParen) {
                                        // Consume the whole parenthesis
                                        for(auto j=parenLevel[nParen-1].firstToken;j<parenLevel[nParen-1].lastToken;++j) {
                                            // Apply the current exponent to all the items in the parenthesis
                                            allTokens[j].expNum *= num;
                                            allTokens[j].expDen *= den;
                                        }
                                        --nParen;
                                        lastTokenWasParen=false;
                                    } else {
                                        // Just the last token
                                        allTokens[nTokens-1].expNum *= num;
                                        allTokens[nTokens-1].expDen *= den;
                                    }
                                    tokStart=tokEnd=i;
                                    if(needDen && u_def[i-1]=='/') {
                                        --i;
                                    }
                                    // Reset next exponent
                                    expnum=1;
                                    expden=1;

                                    continue;
                                }
                            }
                        }
                    }
                }
            }
            ++i;
        }
        if(lastTokenWasParen) {
            // Consume the whole parenthesis
            if(nParen>0) {
                for(auto j=parenLevel[nParen-1].firstToken;j<parenLevel[nParen-1].lastToken;++j) {
                    // Apply the current exponent to all the items in the parenthesis
                    allTokens[j].expNum *= expnum;
                    allTokens[j].expDen *= expden;
                }
            }
            --nParen;
            lastTokenWasParen=false;
        } else {
            // Did we have an open token?
            if(i>tokStart && tokStart!=M-1) {
                tokEnd=i;
                if(i==M)
                    tokEnd--;
                // Consume the token
                allTokens[nTokens]={tokStart,tokEnd,expnum,expden};
                ++nTokens;
                tokStart=tokEnd=i+1;
            }
        }


        // Update Initial Error State
        error_state=UnitError::NoError;
        error_index=0;
        if(nParen!=0) {
            error_state=UnitError::BadParenthesis;
            error_index=M-1;
        }

        for(auto i=0;i<nTokens;++i)
        {
            if(allTokens[i].tokStart == allTokens[i].tokEnd) {
                error_state=UnitError::InvalidToken;
                error_index=allTokens[i].tokStart;
                break;
            } else {
                for(auto j=allTokens[i].tokStart;j<allTokens[i].tokEnd;++j) {
                    // No numbers in a token, otherwise it's fair game to use Unicode chars (Angstrom, Micron, etc.)
                    if(u_def[j]>='0' && u_def[j]<='9') {
                        error_state=UnitError::InvalidToken;
                        error_index=allTokens[i].tokStart+j;
                        break;
                    }
                }
            }
            if(allTokens[i].expDen==0) {
                error_state=UnitError::InvalidExponent;
                error_index=allTokens[i].tokEnd;
                break;
            }
        }

        value_ip=intPart;
        value_exp=-expTen;
        value_den=denPart;
        definition=allTokens;
    }

    // Min operations with units, all create new UnitDefinition objects and are strictly consteval

    // Inverse of a unit U^(-1)
    _CONSTEVAL_ UnitDefinition invert() const {
        UnitDefinition result=*this;
        result.value_ip = value_den;
        result.value_den = value_ip;
        for(size_t i=0;i<maxTokens;++i) {
            result.definition[i].expNum=-result.definition[i].expNum;
            if(definition[i].tokStart==definition[i].tokEnd) break;
        }
        return result;
    }

    // Divide two different units
    _CONSTEVAL_ UnitDefinition divide(const UnitDefinition& other) const {
        UnitDefinition inverse = other.invert();
        return multiply(inverse);
    }

    // Multiply two different units
    _CONSTEVAL_ UnitDefinition multiply(const UnitDefinition& other) const {
        UnitDefinition result = *this;

        // We'll need to add to the definition string to keep the token strings, find the end
        size_t location;
        for(location=0;u_def[location]!=0 && location<maxDefinitionLength;++location);
        // We'll need to add tokens at the end of the definition, find the end
        size_t ntokens;
        for(ntokens=0;definition[ntokens].tokEnd>definition[ntokens].tokStart && ntokens<maxTokens;++ntokens);


        for(size_t i=0;i<maxTokens;++i) {
            auto otherlen=other.definition[i].tokEnd-other.definition[i].tokStart;
            if(otherlen==0) break;
            size_t j;
            size_t len;
            for(j=0;j<maxTokens;++j) {
                len = definition[j].tokEnd-definition[j].tokStart;
                if(len==0) {
                    break;
                }
                if(len!=otherlen) {
                    continue;
                }
                size_t k;
                for(k=0;k<len;++k) {
                    if(u_def[definition[j].tokStart+k]!=other.u_def[other.definition[i].tokStart+k]) {
                        break;
                    }
                }
                if(k==len) {
                    // Found the symbol, search no more
                    break;
                }
            }
            if(j!=maxTokens && len>0) {
                // Found the symbol, so add the exponents
                auto numDenPair = addfraction(definition[j].expNum,definition[j].expDen,other.definition[i].expNum,other.definition[i].expDen);
                result.definition[j].expNum=numDenPair.first;
                result.definition[j].expDen=numDenPair.second;
            }
            else {
                // This symbol is not on the definition, will need to add it
                if(location+otherlen>=maxDefinitionLength) {
                    result.error_index=0;
                    result.error_state=UnitError::DefinitionTooLong;
                    return result;
                }
                for(size_t k=0;k<otherlen;++k) {
                    result.u_def[location+k]=other.u_def[other.definition[i].tokStart+k];
                }
                auto newTokenStart=location;
                location+=otherlen;
                auto newTokenEnd=location;
                result.u_def[location]=0;

                if(ntokens+1>maxTokens) {
                    result.error_index=0;
                    result.error_state=UnitError::TooManyTokens;
                    return result;
                }

                result.definition[ntokens].expNum=other.definition[i].expNum;
                result.definition[ntokens].expDen=other.definition[i].expDen;
                result.definition[ntokens].tokStart=newTokenStart;
                result.definition[ntokens].tokEnd=newTokenEnd;
                ++ntokens;
            }

        }
        // All tokens were processed, eliminate any tokens that have zero exponent
        size_t src,dest;
        for(src=0,dest=0;src<maxTokens;++src) {
            if(result.definition[src].expNum==0) {
                continue;
            }
            if(src!=dest) {
                result.definition[dest]=result.definition[src];
            }
            ++dest;
        }
        result.definition[dest]={0,0,0,0};

        // Multiply the values of the two
        // Watch out for integer overflow in these multiplications
        result.value_ip*=other.value_ip;
        result.value_den*=other.value_den;
        result.value_exp+=other.value_exp;

        // definition string needs to be regenerated
        //        result.regeneratestring();
        return result;
    }

    // Apply a fractional exponent to a unit
    _CONSTEVAL_ UnitDefinition pow(int64_t expNum, int64_t expDen) const {
        UnitDefinition result = *this;
        if(expDen==1) {
            // TODO: Watch for integer overflow here! Even though unit exponents tend to be small numbers, the integer constant can be a big number
            if(expNum<0) {
                result.value_den = intpow(value_ip,-expNum);
                result.value_exp =expNum*value_exp;
                result.value_ip = intpow(value_den,-expNum);
            } else {
            result.value_ip = intpow(value_ip,expNum);
            result.value_exp *=expNum;
            result.value_den = intpow(value_den,expNum);
            }
        }
        else {
            // Fractional powers, use floationg point then come back to integers
            // TODO: Either do this in doubles or use a decimal number library
        }

        for(size_t i=0;i<maxTokens;++i) {
            result.definition[i].expNum*=expNum;
            result.definition[i].expDen*=expDen;
            if(definition[i].tokStart==definition[i].tokEnd) break;
        }
        return result;
    }

    // Simplify a unit by searching through all unit definitions and recursively
    // replacing each named unit with its definition until no more replacements
    // are possible. This produces an equivalent complex unit where all units were
    // reduced to base units
    // Defined outside of the class in order to have the entire unit definition list
    _CONSTEVAL_ UnitDefinition simplify() const;

    // Creates a unit with a single unit token and exponent, taken from the current object
    _CONSTEVAL_ UnitDefinition extractToken(size_t index) const {
        UnitDefinition result;
        result.definition[0]=definition[index];
        size_t tokenLen=definition[index].tokEnd-definition[index].tokStart;
        for(size_t i=0;i<tokenLen;++i) {
            result.u_def[i]=u_def[definition[index].tokStart+i];
        }
        result.u_def[tokenLen]=0;
        result.definition[0].tokStart=0;
        result.definition[0].tokEnd=tokenLen;
        return result;
    }

    // Update the text for the unit definition after a unit was operated upon
    // Uses the list of tokens and exponents to recreate the string.
    _CONSTEVAL_ UnitDefinition update() const {
        // Rebuild the u_def string based off the token list and exponents
        // TODO
        UnitDefinition result;
        result.value_ip=value_ip;
        result.value_den=value_den;
        result.value_exp=value_exp;
        result.error_state=error_state;
        result.error_index=0; // Changing the u_def member will reset this to zero

        // Add the tokens with positive exponents first
        size_t j=0;
        size_t txtj=0;
        for(size_t i=0;i<maxTokens;++i) {
            if(definition[i].tokEnd==definition[i].tokStart) {
                break;
            }
            if(definition[i].expNum>0) {
                // Copy the token to the result
                result.definition[j].expNum=definition[i].expNum;
                result.definition[j].expDen=definition[i].expDen;
                result.definition[j].tokStart=txtj;
                result.definition[j].tokEnd=txtj+definition[i].tokEnd-definition[i].tokStart;
                ++j;
                // Add the token as a string
                if(txtj>0) {
                    result.u_def[txtj++]='*';
                }
                for(auto k=definition[i].tokStart;k<definition[i].tokEnd;++k) {
                    result.u_def[txtj]=u_def[k];
                    ++txtj;
                }
                // Add the exponent to the string
                if(definition[i].expNum!=1 || definition[i].expDen!=1) {
                    // Need to add an exponent
                    result.u_def[txtj++]='^';
                    //
                    int64_t pow10max=1'000'000'000'000'000'000LL;
                    int64_t number=definition[i].expNum;
                    bool firstNonZeroDigit=false;
                    while(number>0) {
                        char digit='0';
                        while(number>=pow10max) {
                            ++digit;
                            number-=pow10max;
                        }
                        if(digit!='0' || firstNonZeroDigit) {
                            result.u_def[txtj++]=digit;
                            firstNonZeroDigit=true;
                        }
                        pow10max/=10;
                    }
                    if(definition[i].expDen!=1) {
                        result.u_def[txtj++]='/';
                        pow10max=1'000'000'000'000'000'000LL;
                        number=definition[i].expDen;
                        bool firstNonZeroDigit=false;
                        while(number>0) {
                            char digit='0';
                            while(number>=pow10max) {
                                ++digit;
                                number-=pow10max;
                            }
                            if(digit!='0' || firstNonZeroDigit) {
                                result.u_def[txtj++]=digit;
                                firstNonZeroDigit=true;
                            }
                            pow10max/=10;
                        }

                    }
                }
            }
        }
        // Second pass, add all of the tokens with negative exponents
        for(size_t i=0;i<maxTokens;++i) {
            if(definition[i].tokEnd==definition[i].tokStart) {
                break;
            }
            if(definition[i].expNum<0) {
                // Copy the token to the result
                result.definition[j].expNum=definition[i].expNum;
                result.definition[j].expDen=definition[i].expDen;
                // Add the token as a string
                if(txtj>0) {
                    result.u_def[txtj++]='/';
                }
                result.definition[j].tokStart=txtj;
                result.definition[j].tokEnd=txtj+definition[i].tokEnd-definition[i].tokStart;
                ++j;
                for(auto k=definition[i].tokStart;k<definition[i].tokEnd;++k) {
                    result.u_def[txtj]=u_def[k];
                    ++txtj;
                }
                // Add the exponent to the string
                if(definition[i].expNum!=-1 || definition[i].expDen!=1) {
                    // Need to add an exponent
                    result.u_def[txtj++]='^';
                    //
                    int64_t pow10max=1'000'000'000'000'000'000LL;
                    int64_t number=-definition[i].expNum;
                    bool firstNonZeroDigit=false;
                    while(number>0) {
                        char digit='0';
                        while(number>=pow10max) {
                            ++digit;
                            number-=pow10max;
                        }
                        if(digit!='0' || firstNonZeroDigit) {
                            result.u_def[txtj++]=digit;
                            firstNonZeroDigit=true;
                        }
                        pow10max/=10;
                    }
                    if(definition[i].expDen!=1) {
                        result.u_def[txtj++]='/';
                        pow10max=1'000'000'000'000'000'000LL;
                        number=definition[i].expDen;
                        bool firstNonZeroDigit=false;
                        while(number>0) {
                            char digit='0';
                            while(number>=pow10max) {
                                ++digit;
                                number-=pow10max;
                            }
                            if(digit!='0' || firstNonZeroDigit) {
                                result.u_def[txtj++]=digit;
                                firstNonZeroDigit=true;
                            }
                            pow10max/=10;
                        }

                    }
                }
            }
        }
        result.u_def[txtj++]=0;
        result.u_defLen=txtj;
        return result;
    }

    // Name of the unit in question (abbreviated form used in formulae, not a formal name, like "Pa" for Pascals)
    // Can be an empty string, an unnamed unit
    char u_name[maxTokenLength]={};
    // Definition of the unit as text: use "1" for base units, or a proper definition in terms of other units for
    // derived units
    char u_def[maxDefinitionLength]={};
    size_t u_defLen = 0;
    // The numerical value of the definition, expressed as a fraction and a base-10 exponent: numerator/denominator * 10^exponent
    int64_t value_ip;
    int64_t value_den;
    int64_t value_exp;
    // Error tracking for run-time error checking or static asserts
    enum UnitError error_state;
    // Position within the definition string where the parser found the error
    size_t error_index;
    // List of tokens and exponents extracted from the definition
    // List ends with a token where start==end
    std::array<tokendata,maxTokens> definition;
};

// Now we include the definition of all units here
#include "allunits.hpp"

// This is the only member of UnitDefinition defined outside the class definition
// because it needs the list of all units to exist as a _CONSTEXPR_ to replace the units
// with their definitions
// For example, if the definitions are:
// "?m"="1" (base unit for length, accepting SI prefixes)
// "in"="25.4_mm"
// then the expression "in^2/m^2" will be expanded to:
// "(25.4_mm)^2/m^2" the unit in is replaced with its definition,
// while the meter is not replaced because it's a base unit
// The next pass becomes "25.4^2*(1/1000_m)^2/m^2" as the "mm" unit gets
// replaced by extracting the SI prefix
// Finally, the exponents for the 'm' are added, and the whole token
// removed because the addition results in a zero exponent
// becoming "25.4^2/1000^2"

_CONSTEVAL_ UnitDefinition UnitDefinition::simplify() const
{
    UnitDefinition result=*this;

    bool expansionHappened;

    do {
        expansionHappened = false;
        UnitDefinition expanded;
        // Take only the value, we'll fill out the units later
        expanded.value_ip=result.value_ip;
        expanded.value_den=result.value_den;
        expanded.value_exp=result.value_exp;

        // Scan through all the units, expand any that exists in the table
        for(size_t i=0;i<maxTokens;++i) {
            size_t tokenLen=result.definition[i].tokEnd-result.definition[i].tokStart;
            if(!tokenLen) break;
            // See if a token exists in the table, search by name
            bool isAMatch=false;
            for(size_t j=0;j<sizeof(allUnits)/sizeof(UnitDefinition) && allUnits[j].value_ip!=0;++j) {
                size_t namestart=allUnits[j].u_name[0]=='?'? 1:0;
                isAMatch=true;
                // Compare the name
                for(size_t k=0;k<maxTokenLength && result.definition[i].tokStart+k<result.definition[i].tokEnd;++k) {
                    if(result.u_def[result.definition[i].tokStart+k]!=allUnits[j].u_name[namestart+k]) {
                        isAMatch=false;
                        break;
                    }
                }
                if(isAMatch) {
                    // Only replace if it's a derived unit
                    if(allUnits[j].definition[0].tokStart==allUnits[j].definition[0].tokEnd) {
                        // This is a base unit, do NOT replace
                        isAMatch=false;
                        break;
                    }
                    else {
                        // Apply the exponent to the definition
                        UnitDefinition replacement = allUnits[j].pow(result.definition[i].expNum,result.definition[i].expDen);
                        // And multiply it to our result
                        expanded = expanded.multiply(replacement);
                        expansionHappened=true;
                    }
                } else {
                    // If this unit supports SI prefixes, we need to do a second comparison
                    if(namestart>0) {
                        // TODO: Strip the SI prefix from the unit and compare again
                        size_t newtokenStart=result.definition[i].tokStart;
                        _CONSTEXPR_ char siPrefixes[]="QRYZEPTGMkhdcmnpfazyrq";
                        int64_t siPrefixExponent=0;
                        size_t si;
                        for(si=0;siPrefixes[si]!=0;++si) {
                            if(result.u_def[result.definition[i].tokStart]==siPrefixes[si]) {
                                break;
                            }
                        }
                        // If it was one of the letters, skip it
                        // Handle 2 special cases: deka and micro
                        if(siPrefixes[si]!=0) {
                            newtokenStart++;
                            if(si<=9) {
                                siPrefixExponent=30-3*si;
                            }
                            if(si==10) {
                                siPrefixExponent=2;
                            }
                            if(si==11) {
                                siPrefixExponent=-1;
                            }
                            if(si==12) {
                                siPrefixExponent=-2;
                            }
                            if(si==13) {
                                siPrefixExponent=-3;
                            }
                            if(si>=14) {
                                siPrefixExponent=-9-3*(si-14);
                            }
                            if(siPrefixes[si]=='d' && result.u_def[newtokenStart+1]=='a' && tokenLen>2) {
                                ++newtokenStart;
                                siPrefixExponent=1;
                            }
                        }
                        else {
                            // For Micro, we need to look for the Unicode symbol in UTF-8
                            // It can be U+03bc=={0xce, 0xbc} or U+00B5=={0xc2,0xb5}
                            if(tokenLen>2 && (unsigned char)(result.u_def[newtokenStart])==0xc2 && (unsigned char)(result.u_def[newtokenStart])==0xbc) {
                                newtokenStart+=2;
                                siPrefixExponent=-6;
                            }
                            if(tokenLen>2 && (unsigned char)result.u_def[newtokenStart]==0xce && (unsigned char)result.u_def[newtokenStart]==0xb5) {
                                newtokenStart+=2;
                                siPrefixExponent=-6;
                            }
                        }

                        // Do the whole comparison once more
                        isAMatch=true;
                        size_t k;
                        for(k=0;k<maxTokenLength && newtokenStart+k<result.definition[i].tokEnd;++k) {
                            if(result.u_def[newtokenStart+k]!=allUnits[j].u_name[namestart+k]) {
                                isAMatch=false;
                                break;
                            }
                            if(result.u_def[newtokenStart+k]==0) {
                                break;
                            }
                        }
                        if(allUnits[j].u_name[namestart+k]!=0) {
                            isAMatch=false;
                        }
                        if(isAMatch) {
                            // Apply the exponent to the definition
                            UnitDefinition prefixedUnit = allUnits[j];
                            if(prefixedUnit.definition[0].tokStart==prefixedUnit.definition[0].tokEnd) {
                                // This is a base unit, with an SI prefix applied
                                // Replace with the base unit name, skipping the '?' prefix in the name
                                size_t t;
                                for(t=0;prefixedUnit.u_name[t+1]!=0;++t) {
                                    prefixedUnit.u_def[t]=prefixedUnit.u_name[t+1];
                                }
                                prefixedUnit.u_def[t]=0;
                                prefixedUnit.definition[0]={0,t,1,1};
                            }
                            prefixedUnit.value_exp+=siPrefixExponent;
                            UnitDefinition replacement = prefixedUnit.pow(result.definition[i].expNum,result.definition[i].expDen);
                            // And multiply it to our result
                            expanded = expanded.multiply(replacement);
                            expansionHappened=true;
                            break;
                        }

                    }
                }
            }
            if(!isAMatch) {
                // We need to add this token to the expansion, doesn't alter the value
                // Apply the exponent to the definition
                UnitDefinition replacement = result.extractToken(i);
                // And multiply it to our result
                expanded = expanded.multiply(replacement);
            }
        }
        result=expanded;
    } while(expansionHappened);

    return result;
}

template<size_t N>
_CONSTEVAL_ auto to_UnitPart(const UnitDefinition def)
{
    char result[N]{};
    std::copy_n(def.u_def, N, result);
    return UnitPart<N>{result};
}


// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************

// This is the main 'Quantity' class, with a string as a template argument
// that represents the unit definition
template<UnitPart U>
struct Qty {
    // Default constructor creates the non-dimensional number 0.0
    _CONSTEXPR_ inline Qty() : value(0.0) {}
    // Constructor with an integer value
    _CONSTEXPR_ inline Qty(int64_t _value) : value((double)_value) {}
    // Constructor with a floating point value
    _CONSTEXPR_ inline Qty(double _value) : value(_value) {}
    // Constructor for temporaries
    _CONSTEXPR_ inline Qty(long double _value) : value(_value) {}

    // Calculate a multiplicative conversion factor to convert from
    // the current unit to the unit of the given argument
    template<UnitPart V>
    _CONSTEVAL_ long double conversionFactorTo(const Qty<V>&) const {
        // To convert a unit from U to V:
        // k*U = k*U* (expand(U)/U) * (V/expand(V)) = k*V* (expand(U)/expand(V))
        // k*U = k*V * (expand(U)/expand(V))
        // conv. factor = (expand(U)/expand(V))

        // Create a unit V with a value of 1 that is a _CONSTEXPR_
        // We cannot use the one from the argument because the quantity may not be _CONSTEXPR_
        // even though the unit of the quantity always is
        _CONSTEXPR_ Qty<V> otherunit{1.0};
        _CONSTEXPR_ auto quotientUnit = unit.divide(otherunit.unit); // This is (U / V)
        _CONSTEXPR_ auto combinedUnit=quotientUnit.simplify(); // This makes expand(U/V) == expand(U)/expand(V) == conversion factor
        // After simplification, the result should be a non-dimensional factor, otherwise the units are
        // incompatible and we cannot convert
        // Check that the resulting unit has an 'empty' list of tokens (therefore non-dimensional)
        if(combinedUnit.definition[0].tokEnd!=combinedUnit.definition[0].tokStart) {
            // Also check that there were no errors
            if(combinedUnit.error_state!=UnitError::NoError) {
                throw std::runtime_error(UnitErrorMessages[combinedUnit.error_state]);
            }
            // This isn't a real throw, just generates a compile error. The string will be buried in other
            // messages but still visible by the user
            throw "Incompatible Units";
        }
        // Units were compatible, so extract the value
        _CONSTEXPR_ long double convFactor=(combinedUnit.value_den!=1)? ((long double)combinedUnit.value_ip) / combinedUnit.value_den : combinedUnit.value_ip;
        // Apply the exponent
        long double powerOf10=1.0;
        if(combinedUnit.value_exp) {
            if(intabs(combinedUnit.value_exp)<18) {
                int64_t ipowerOf10=1;
                // Should use a table for this, but for now...
                for(auto k=0;k<intabs(combinedUnit.value_exp);++k) {
                    ipowerOf10*=10;
                }
                if(combinedUnit.value_exp<0) {
                    powerOf10/=ipowerOf10;
                } else {
                    powerOf10*=ipowerOf10;
                }
            }
            else {
                powerOf10 = std::pow(10.0,combinedUnit.value_exp);
            }
        }
        // And we have the conversion factor
        return convFactor*powerOf10;
    }

    // Generic unit conversion operator, _CONSTEXPR_ will be used automatically
    template<UnitPart V>
    _CONSTEXPR_ operator Qty<V>() const {
        // Create guaranteed _CONSTEXPR_ units for source and destination
        _CONSTEXPR_ Qty<V> unitV{1.0};
        _CONSTEXPR_ Qty<U> unitU{1.0};
        // Calculate a conversion factor
        _CONSTEXPR_ auto convFactor = unitU.conversionFactorTo(unitV);
        // Return a completely new object with the requested unit
        // here we multiply the value by the conversion factor
        // with the caveat that the value may not be _CONSTEXPR_
        // so the compiler may issue a single multiplication
        // to do the conversion at run time
        return Qty<V>{value*convFactor};
    }

    // The actual unit for the physical quantity is a static _CONSTEXPR_ member of the class
    // therefore the compiler will not create/store any data unless it is used during
    // run time
    static constexpr UnitDefinition unit={U};
    // This becomes the one and only data member: the value
    long double value;
};

// Addition operator for 2 units
// Convention: Resulting unit of an addition or subtraction is always the unit of the left argument
// This is done to guarantee that adding with += operations do not change the unit of the result
template <UnitPart U, UnitPart V>
_CONSTEXPR_ Qty<U> operator+(const Qty<U>& lhs, const Qty<V>& rhs) {
    const auto convFactor = rhs.conversionFactorTo(lhs);
    long double finalvalue = lhs.value + rhs.value * convFactor;
    return Qty<U>{finalvalue};
}

template <UnitPart U, UnitPart V>
_CONSTEXPR_ Qty<U> operator-(const Qty<U>& lhs, const Qty<V>& rhs) {
    const auto convFactor = rhs.conversionFactorTo(lhs);
    long double finalvalue = lhs.value - rhs.value * convFactor;
    return Qty<U>{finalvalue};
}

// Multiplication operator for 2 units
// No unit conversion is performed, the values are simply multiplied
// The resulting unit is not simplified in any way
// except identical tokens will merge their exponents
// For example: m*m^2 == m^3 but m*cm == m*cm (the SI prefixed unit is
// treated as a different unit)
template <UnitPart U, UnitPart V>
_CONSTEXPR_ auto operator*(const Qty<U>& lhs, const Qty<V>& rhs) {
    // Guarantee all _CONSTEXPR_ constants so the operation is fully constevaled
    // even if the arguments have unknown values at compile time
    _CONSTEXPR_ Qty<U> lhsUnit{1.0};
    _CONSTEXPR_ Qty<V> rhsUnit{1.0};
    _CONSTEXPR_ auto finalUnit = lhsUnit.unit.multiply(rhsUnit.unit).update();
    constexpr auto finalUnitDefinition = to_UnitPart<finalUnit.u_defLen>(finalUnit);
    // This it the only operation the compiler will do at run time if needed
    long double finalvalue = lhs.value * rhs.value;
    return Qty<finalUnitDefinition>{finalvalue};
}

template <UnitPart U, UnitPart V>
_CONSTEXPR_ auto operator/(const Qty<U>& lhs, const Qty<V>& rhs) {
    // Guarantee all _CONSTEXPR_ constants so the operation is fully constevaled
    // even if the arguments have unknown values at compile time
    _CONSTEXPR_ Qty<U> lhsUnit{1.0};
    _CONSTEXPR_ Qty<V> rhsUnit{1.0};
    _CONSTEXPR_ auto finalUnit = lhsUnit.unit.divide(rhsUnit.unit).update();
    constexpr auto finalUnitDefinition = to_UnitPart<finalUnit.u_defLen>(finalUnit);

    // This it the only operation the compiler will do at run time if needed
    long double finalvalue = lhs.value / rhs.value;
    return Qty<finalUnitDefinition>{finalvalue};
}


// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
template <UnitPart V>
_CONSTEXPR_ Qty<V> operator*(const long double lhs, const Qty<V>& rhs) {
    long double finalvalue = lhs * rhs.value;
    return Qty<V>{finalvalue};
}

template <UnitPart V>
_CONSTEXPR_ Qty<V> operator*(const double lhs, const Qty<V>& rhs) {
    long double finalvalue = lhs * rhs.value;
    return Qty<V>{finalvalue};
}

// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
template <UnitPart V>
_CONSTEXPR_ Qty<V> operator*(const int64_t lhs, const Qty<V>& rhs) {
    long double finalvalue = lhs * rhs.value;
    return Qty<V>{finalvalue};
}

// Multiplication operator by a non-dimensional scalar
// Simply preserves the original unit
template <UnitPart V>
_CONSTEXPR_ Qty<V> operator*(const int lhs, const Qty<V>& rhs) {
    long double finalvalue = lhs * rhs.value;
    return Qty<V>{finalvalue};
}


// This is a hack to create constants in a user-friendly way, I don't like it much
#define _(TXT) Qty< TXT >{1.0}
