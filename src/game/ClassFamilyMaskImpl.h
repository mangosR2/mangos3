/*
 * Copyright (C) 2011-2012 MangosR2 <http://github.com/MangosR2>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MANGOS_CLASSFAMILYMASK_H
#define MANGOS_CLASSFAMILYMASK_H

// template arguments for declaration
#define CFM_ARGS_1  ClassFlag N1
#define CFM_ARGS_2  CFM_ARGS_1, ClassFlag N2
#define CFM_ARGS_3  CFM_ARGS_2, ClassFlag N3
#define CFM_ARGS_4  CFM_ARGS_3, ClassFlag N4
#define CFM_ARGS_5  CFM_ARGS_4, ClassFlag N5
#define CFM_ARGS_6  CFM_ARGS_5, ClassFlag N6
#define CFM_ARGS_7  CFM_ARGS_6, ClassFlag N7
#define CFM_ARGS_8  CFM_ARGS_7, ClassFlag N8
#define CFM_ARGS_9  CFM_ARGS_8, ClassFlag N9
#define CFM_ARGS_10 CFM_ARGS_9, ClassFlag N10

// template values for function calls
#define CFM_VALUES_1  N1
#define CFM_VALUES_2  CFM_VALUES_1, N2
#define CFM_VALUES_3  CFM_VALUES_2, N3
#define CFM_VALUES_4  CFM_VALUES_3, N4
#define CFM_VALUES_5  CFM_VALUES_4, N5
#define CFM_VALUES_6  CFM_VALUES_5, N6
#define CFM_VALUES_7  CFM_VALUES_6, N7
#define CFM_VALUES_8  CFM_VALUES_7, N8
#define CFM_VALUES_9  CFM_VALUES_8, N9
#define CFM_VALUES_10 CFM_VALUES_9, N10

struct ClassFamilyMask
{
    uint64 Flags;
    uint32 Flags2;

    ClassFamilyMask() : Flags(0), Flags2(0) {}
    explicit ClassFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) : Flags(familyFlags), Flags2(familyFlags2) {}
    ClassFamilyMask(uint32 f0, uint32 f1, uint32 f2): Flags(uint64(f0) | (uint64(f1) << 32)), Flags2(f2) {}

    // predefined empty object for safe return by reference
    static ClassFamilyMask const Null;

    bool Empty() const { return Flags == 0 && Flags2 == 0; }
    bool operator! () const { return Empty(); }
    operator void const* () const { return Empty() ? NULL : this; }// for allow normal use in if(mask)

    bool IsFitToFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        return (Flags & familyFlags) || (Flags2 & familyFlags2);
    }

    bool IsFitToFamilyMask(ClassFamilyMask const& mask) const
    {
        return (Flags & mask.Flags) || (Flags2 & mask.Flags2);
    }

    uint64 operator& (uint64 mask) const                     // possible will removed at finish convertion code use IsFitToFamilyMask
    {
        return Flags & mask;
    }

    // test if specified bits are set (run-time)
    bool test(size_t offset) const
    {
        return reinterpret_cast<uint8 const*>(this)[offset >> 3] & (uint8(1) << (offset & 7));
    }

    // test if specified bits are set (compile-time)
    template <CFM_ARGS_1>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_1>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_1>::value);
    }

    template <CFM_ARGS_2>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_2>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_2>::value);
    }

    template <CFM_ARGS_3>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_3>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_3>::value);
    }

    template <CFM_ARGS_4>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_4>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_4>::value);
    }

    template <CFM_ARGS_5>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_5>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_5>::value);
    }

    template <CFM_ARGS_6>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_6>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_6>::value);
    }

    template <CFM_ARGS_7>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_7>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_7>::value);
    }

    template <CFM_ARGS_8>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_8>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_8>::value);
    }

    template <CFM_ARGS_9>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_9>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_9>::value);
    }

    template <CFM_ARGS_10>
    bool test() const
    {
        return (Flags  & BitMask<uint64, true,  CFM_VALUES_10>::value) ||
               (Flags2 & BitMask<uint32, false, CFM_VALUES_10>::value);
    }

    // named constructors (compile-time)
    template <CFM_ARGS_1>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_1>::value,
                               BitMask<uint32, false, CFM_VALUES_1>::value);
    }

    template <CFM_ARGS_2>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_2>::value,
                               BitMask<uint32, false, CFM_VALUES_2>::value);
    }

    template <CFM_ARGS_3>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_3>::value,
                               BitMask<uint32, false, CFM_VALUES_3>::value);
    }

    template <CFM_ARGS_4>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_4>::value,
                               BitMask<uint32, false, CFM_VALUES_4>::value);
    }

    template <CFM_ARGS_5>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_5>::value,
                               BitMask<uint32, false, CFM_VALUES_5>::value);
    }

    template <CFM_ARGS_6>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_6>::value,
                               BitMask<uint32, false, CFM_VALUES_6>::value);
    }

    template <CFM_ARGS_7>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_7>::value,
                               BitMask<uint32, false, CFM_VALUES_7>::value);
    }

    template <CFM_ARGS_8>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_8>::value,
                               BitMask<uint32, false, CFM_VALUES_8>::value);
    }

    template <CFM_ARGS_9>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_9>::value,
                               BitMask<uint32, false, CFM_VALUES_9>::value);
    }

    template <CFM_ARGS_10>
    static ClassFamilyMask create()
    {
        return ClassFamilyMask(BitMask<uint64, true,  CFM_VALUES_10>::value,
                               BitMask<uint32, false, CFM_VALUES_10>::value);
    }

    // comparison operators
    bool operator== (ClassFamilyMask const& rhs) const
    {
        return Flags == rhs.Flags && Flags2 == rhs.Flags2;
    }

    bool operator!= (ClassFamilyMask const& rhs) const
    {
        return Flags != rhs.Flags || Flags2 != rhs.Flags2;
    }

    // bitwise operators
    ClassFamilyMask operator& (ClassFamilyMask const& rhs) const
    {
        return ClassFamilyMask(Flags & rhs.Flags, Flags2 & rhs.Flags2);
    }

    ClassFamilyMask operator| (ClassFamilyMask const& rhs) const
    {
        return ClassFamilyMask(Flags | rhs.Flags, Flags2 | rhs.Flags2);
    }

    ClassFamilyMask operator^ (ClassFamilyMask const& rhs) const
    {
        return ClassFamilyMask(Flags ^ rhs.Flags, Flags2 ^ rhs.Flags2);
    }

    ClassFamilyMask operator~ () const
    {
        return ClassFamilyMask(~Flags, ~Flags2);
    }

    // assignation operators
    ClassFamilyMask& operator= (ClassFamilyMask const& rhs)
    {
        Flags  = rhs.Flags;
        Flags2 = rhs.Flags2;
        return *this;
    }

    ClassFamilyMask& operator&= (ClassFamilyMask const& rhs)
    {
        Flags  &= rhs.Flags;
        Flags2 &= rhs.Flags2;
        return *this;
    }

    ClassFamilyMask& operator|= (ClassFamilyMask const& rhs)
    {
        Flags  |= rhs.Flags;
        Flags2 |= rhs.Flags2;
        return *this;
    }

    ClassFamilyMask& operator^= (ClassFamilyMask const& rhs)
    {
        Flags  ^= rhs.Flags;
        Flags2 ^= rhs.Flags2;
        return *this;
    }

    // templates used for compile-time mask calculation
private:
    enum { LOW_WORD_SIZE = 64 };

    template <typename T, int Val, bool IsLow, bool InRange>
    struct DoShift
    {
        static T const value = T(1) << Val;
    };

    template <typename T, int Val>
    struct DoShift<T, Val, false, true>
    {
        static T const value = T(1) << (Val - LOW_WORD_SIZE);
    };

    template <typename T, int Val, bool IsLow>
    struct DoShift<T, Val, IsLow, false>
    {
        static T const value = 0;
    };

    template <int N, bool IsLow>
    struct IsInRange
    {
        static bool const value = IsLow ? N < LOW_WORD_SIZE : N >= LOW_WORD_SIZE;
    };

    template <typename T, bool IsLow, int N1, int N2 = -1, int N3 = -1, int N4 = -1, int N5 = -1, int N6 = -1, int N7 = -1, int N8 = -1, int N9 = -1, int N10 = -1>
    struct BitMask
    {
        static T const value = DoShift<T, N1, IsLow, IsInRange<N1, IsLow>::value>::value | BitMask<T, IsLow, N2, N3, N4, N5, N6, N7, N8, N9, N10, -1>::value;
    };

    template <typename T, bool IsLow>
    struct BitMask<T, IsLow, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1>
    {
        static T const value = 0;
    };
};

#endif //MANGOS_CLASSFAMILYMASK_H