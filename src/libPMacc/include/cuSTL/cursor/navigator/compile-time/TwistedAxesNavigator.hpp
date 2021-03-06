/**
 * Copyright 2013 Heiko Burau, Rene Widera
 *
 * This file is part of libPMacc.
 *
 * libPMacc is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libPMacc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with libPMacc.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CURSOR_CT_TWISTEDAXESNAVIGATOR_HPP
#define CURSOR_CT_TWISTEDAXESNAVIGATOR_HPP

#include "math/Vector.hpp"

namespace PMacc
{
namespace cursor
{
namespace CT
{

template<typename Axes, int dim = Axes::dim>
struct TwistedAxesNavigator;

template<typename Axes>
struct TwistedAxesNavigator<Axes, 2>
{
    static const int dim = 2;

    template<typename TCursor>
    HDINLINE
    TCursor operator()(const TCursor& cursor, const math::Int<2>& jump) const
    {
        math::Int<2> twistedJump;
        twistedJump[Axes::x::value] = jump.x();
        twistedJump[Axes::y::value] = jump.y();
        return cursor(twistedJump);
    }
};

template<typename Axes>
struct TwistedAxesNavigator<Axes, 3>
{
    static const int dim = 3;

    template<typename TCursor>
    HDINLINE
    TCursor operator()(const TCursor& cursor, const math::Int<3>& jump) const
    {
        math::Int<3> twistedJump;
        twistedJump[Axes::x::value] = jump.x();
        twistedJump[Axes::y::value] = jump.y();
        twistedJump[Axes::z::value] = jump.z();
        return cursor(twistedJump);
    }
};

} // CT
} // cursor
} // PMacc

#endif // CURSOR_CT_TWISTEDAXESNAVIGATOR_HPP
