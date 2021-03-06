/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



#include <resources.hxx>

namespace writerfilter {
namespace doctok {

/* WW8sprmTDefTable */

sal_uInt32 WW8sprmTDefTable::get_cellx_count()
{
    return getU8(0x4) + 1;
}

sal_uInt16 WW8sprmTDefTable::get_cellx(sal_uInt32 nIndex)
{
    return getU16(0x5 + nIndex * 2);
}

sal_uInt32 WW8sprmTDefTable::get_tc_count()
{
    return get_cellx_count() - 1;
}

writerfilter::Reference<Properties>::Pointer_t 
WW8sprmTDefTable::get_tc(sal_uInt32 pos)
{
    return writerfilter::Reference<Properties>::Pointer_t
        (new WW8TC(this, 
                   0x5 + get_cellx_count() * 2 + pos * WW8TC::getSize()));
}

/* WW8sprmTTableBorders */

sal_uInt32 WW8sprmTTableBorders::get_rgbbrc_count()
{
    return 6;
}

writerfilter::Reference<Properties>::Pointer_t
WW8sprmTTableBorders::get_rgbbrc(sal_uInt32 pos)
{
    return writerfilter::Reference<Properties>::Pointer_t
        (new WW8BRC(*this, 0x3 + pos * WW8BRC::getSize()));
}

/* WW8sprmTCellBottomColor */

sal_uInt32 WW8sprmTCellBottomColor::calcSize()
{
    return  get_cellBottomColor_count() * 4 + 3;
}

sal_uInt32 WW8sprmTCellBottomColor::get_cellBottomColor_count()
{
    return getU8(0x2) / 4;
}

sal_uInt32 WW8sprmTCellBottomColor::get_cellBottomColor(sal_uInt32 pos)
{
    return getU32(0x3 + 4 * pos);
}

/* WW8sprmTCellLeftColor */

sal_uInt32 WW8sprmTCellLeftColor::calcSize()
{
    return  get_cellLeftColor_count() * 4 + 3;
}

sal_uInt32 WW8sprmTCellLeftColor::get_cellLeftColor_count()
{
    return getU8(0x2) / 4;
}

sal_uInt32 WW8sprmTCellLeftColor::get_cellLeftColor(sal_uInt32 pos)
{
    return getU32(0x3 + 4 * pos);
}

/* WW8sprmTCellTopColor */

sal_uInt32 WW8sprmTCellTopColor::calcSize()
{
    return  get_cellTopColor_count() * 4 + 3;
}

sal_uInt32 WW8sprmTCellTopColor::get_cellTopColor_count()
{
    return getU8(0x2) / 4;
}

sal_uInt32 WW8sprmTCellTopColor::get_cellTopColor(sal_uInt32 pos)
{
    return getU32(0x3 + 4 * pos);
}

/* WW8sprmTCellRightColor */

sal_uInt32 WW8sprmTCellRightColor::calcSize()
{
    return  get_cellRightColor_count() * 4 + 3;
}

sal_uInt32 WW8sprmTCellRightColor::get_cellRightColor_count()
{
    return getU8(0x2) / 4;
}

sal_uInt32 WW8sprmTCellRightColor::get_cellRightColor(sal_uInt32 pos)
{
    return getU32(0x3 + 4 * pos);
}

/* WW8sprmTGridLineProps */

sal_uInt32 WW8sprmTGridLineProps::calcSize()
{
    return getSize();
}

/* WW8sprmTDefTableShd */

sal_uInt32 WW8sprmTDefTableShd::calcSize()
{
    return get_shd_count() * WW8SHD::getSize() + 3;
}

sal_uInt32 WW8sprmTDefTableShd::get_shd_count()
{
    return getU8(0x2) / WW8SHD::getSize();
}

writerfilter::Reference<Properties>::Pointer_t
WW8sprmTDefTableShd::get_shd(sal_uInt32 pos)
{
    return writerfilter::Reference<Properties>::Pointer_t
        (new WW8SHD(*this, 0x3 + pos * WW8SHD::getSize()));
}

/* WW8sprmTCellShd */

sal_uInt32 WW8sprmTCellShd::calcSize()
{
    return get_shd_count() * WW8CellShd::getSize();
}

sal_uInt32 WW8sprmTCellShd::get_shd_count()
{
    return getU8(0x2) / WW8CellShd::getSize();
}

writerfilter::Reference<Properties>::Pointer_t
WW8sprmTCellShd::get_shd(sal_uInt32 pos)
{
    return writerfilter::Reference<Properties>::Pointer_t
        (new WW8CellShd(*this, 0x3 + pos * WW8CellShd::getSize()));
}

/* WW8sprmTCellShadow */

sal_uInt32 WW8sprmTCellShadow::calcSize()
{
    return get_cellShadow_count() * WW8CellShd::getSize();
}

sal_uInt32 WW8sprmTCellShadow::get_cellShadow_count()
{
    return getU8(0x2) / WW8CellShd::getSize();
}

writerfilter::Reference<Properties>::Pointer_t
WW8sprmTCellShadow::get_cellShadow(sal_uInt32 pos)
{
    return writerfilter::Reference<Properties>::Pointer_t
        (new WW8CellShd(*this, 0x3 + pos * WW8CellShd::getSize()));
}

}}
