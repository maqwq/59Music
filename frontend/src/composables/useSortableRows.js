import { ref, onMounted, onUnmounted, watch, nextTick } from 'vue'
import Sortable from 'sortablejs'

/**
 * 表格行拖拽排序 composable
 *
 * 基于 sortablejs，封装 el-table 的行拖拽逻辑。
 *
 * 使用示例：
 *   const tableRef = ref()
 *   useSortableRows(tableRef, {
 *     data: computed(() => list),
 *     onSort: (oldIndex, newIndex) => { ... }
 *   })
 *
 * @param {import('vue').Ref} tableRef el-table 的 ref
 * @param {Object} options
 * @param {import('vue').Ref<any[]>} [options.data] 表格数据，用于数据变化时重新绑定
 * @param {(oldIndex: number, newIndex: number) => void} [options.onSort] 拖拽结束回调
 * @param {string} [options.handle] 拖拽手柄选择器，默认整行可拖拽
 */
export function useSortableRows(tableRef, options = {}) {
  let sortableInstance = null

  function getTbody() {
    return tableRef.value?.$el.querySelector('.el-table__body tbody')
  }

  function initSortable() {
    const tbody = getTbody()
    if (!tbody) return

    // 先销毁旧的，避免重复绑定
    sortableInstance?.destroy()

    sortableInstance = Sortable.create(tbody, {
      animation: 150,
      handle: options.handle || undefined,
      ghostClass: 'sortable-ghost',
      chosenClass: 'sortable-chosen',
      dragClass: 'sortable-drag',
      onEnd: (event) => {
        const { oldIndex, newIndex } = event
        if (oldIndex === newIndex || newIndex == null) return
        options.onSort?.(oldIndex, newIndex)
      },
    })
  }

  function destroySortable() {
    sortableInstance?.destroy()
    sortableInstance = null
  }

  onMounted(() => {
    nextTick(initSortable)
  })

  onUnmounted(() => {
    destroySortable()
  })

  // 数据变化后重新初始化 sortable，因为 el-table 可能会重建 tbody
  if (options.data) {
    watch(
      () => options.data.value,
      () => {
        nextTick(initSortable)
      },
      { deep: true }
    )
  }

  return {
    initSortable,
    destroySortable,
  }
}
