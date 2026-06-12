import { ref, computed, onMounted, onUnmounted, reactive } from 'vue'

/**
 * 多选逻辑 composable
 *
 * 支持：
 * - 单选 / 反选
 * - 全选 / 清空
 * - 半选状态
 * - 鼠标滑动连续选择
 *
 * 使用示例：
 *   const { selectedIds, selectedItems, isSelected, isAllSelected, isIndeterminate,
 *           toggle, selectAll, clear, startDrag, onItemEnter } = useSelection(items, { key: 'id' })
 *
 * @param {import('vue').Ref<any[]>} items 数据列表
 * @param {Object} options
 * @param {string} [options.key='id'] 唯一标识字段
 */
export function useSelection(items, options = {}) {
  const keyField = options.key || 'id'

  /** 当前选中的 key 集合 */
  const selected = ref(new Set())

  /** 是否处于滑动选择模式 */
  const dragging = ref(false)

  /** 滑动开始时，起始项的选中状态（true 表示这次拖动是取消选中） */
  const dragStartSelected = ref(false)

  // ========== 计算属性 ==========

  const selectedItems = computed(() =>
    items.value.filter((item) => selected.value.has(getKey(item)))
  )

  const selectedIds = computed(() => [...selected.value])

  const selectedCount = computed(() => selected.value.size)

  const isAllSelected = computed(() => {
    if (items.value.length === 0) return false
    return items.value.every((item) => selected.value.has(getKey(item)))
  })

  const isIndeterminate = computed(() => {
    const count = selected.value.size
    return count > 0 && count < items.value.length
  })

  // ========== 基础方法 ==========

  function getKey(item) {
    return item[keyField]
  }

  function isSelected(item) {
    return selected.value.has(getKey(item))
  }

  function select(item) {
    selected.value.add(getKey(item))
  }

  function deselect(item) {
    selected.value.delete(getKey(item))
  }

  /** 标记下一次 change 事件是否应被忽略（用于避免拖拽结束后的 click 反转起始项） */
  let ignoreNextChange = false

  function toggle(item) {
    if (ignoreNextChange) {
      ignoreNextChange = false
      return
    }
    if (isSelected(item)) {
      deselect(item)
    } else {
      select(item)
    }
  }

  function selectAll() {
    items.value.forEach((item) => select(item))
  }

  function clear() {
    selected.value.clear()
  }

  function handleSelectAllChange(checked) {
    if (checked) {
      selectAll()
    } else {
      clear()
    }
  }

  // ========== 滑动选择 ==========

  /**
   * 开始滑动选择
   * @param {MouseEvent} event 鼠标事件，用于阻止默认行为
   * @param {any} item 起始项
   */
  function startDrag(event, item) {
    if (event) {
      event.preventDefault()
    }
    dragging.value = true
    dragStartSelected.value = isSelected(item)
    ignoreNextChange = false
  }

  /**
   * 鼠标划入某行时的处理
   * @param {any} item
   */
  function onItemEnter(item) {
    if (!dragging.value) return

    ignoreNextChange = true

    if (dragStartSelected.value) {
      deselect(item)
    } else {
      select(item)
    }
  }

  /**
   * 结束滑动选择
   */
  function endDrag() {
    dragging.value = false
  }

  // 全局监听 mouseup，确保拖拽在任意位置释放都能结束
  onMounted(() => {
    document.addEventListener('mouseup', endDrag)
  })

  onUnmounted(() => {
    document.removeEventListener('mouseup', endDrag)
  })

  return reactive({
    selected,
    selectedItems,
    selectedIds,
    selectedCount,
    isSelected,
    isAllSelected,
    isIndeterminate,
    dragging,
    toggle,
    select,
    deselect,
    selectAll,
    clear,
    handleSelectAllChange,
    startDrag,
    onItemEnter,
    endDrag,
  })
}
