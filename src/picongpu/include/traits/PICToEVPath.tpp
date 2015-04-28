/**
 * Copyright 2013-2015 Anshuman Goswami
 *
 * This file is part of PIConGPU.
 *
 * PIConGPU is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PIConGPU is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIConGPU.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if (ENABLE_EVPATH==1)
#include <evpath.h>

#include "simulation_defines.hpp"

namespace picongpu
{

namespace traits
{
    /** Trait for int */
    template<>
    struct PICToEVPath<int>
    {
        //ADIOS_DATATYPES type;
        
        //PICToAdios() :
        //type(adios_integer) {}

        FMdata_type type;
        PICToEVPath() :
        type(integer_type) {} 
    };
    
    /** Trait for unsigned */
    template<>
    struct PICToEVPath<unsigned>
    {
        FMdata_type type;
        PICToEVPath() :
        type(unsigned_type) {} 
    };
    
    /** Trait for float */
    template<>
    struct PICToEVPath<float>
    {
        FMdata_type type;
        PICToEVPath() :
        type(float_type) {} 
    };

    /** Trait for char */
    template<>
    struct PICToEVPath<char>
    {
        FMdata_type type;
        PICToEVPath() :
        type(char_type) {} 
    };

    /** Trait for bool */
    template<>
    struct PICToEVPath<bool>
    {
        FMdata_type type;
        PICToEVPath() :
        type(boolean_type) {} 
    };

} //namespace traits

}// namespace picongpu

#endif // (ENABLE_ADIOS==1)
