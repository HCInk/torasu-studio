#ifndef SRC_STATE_ELEMENTDISPLAY_HPP_
#define SRC_STATE_ELEMENTDISPLAY_HPP_

#include "TreeManager.hpp"

namespace tstudio {

class ElementDisplay {
public: // Types / Classes
	typedef size_t version_t;

	struct NodePosition {
		enum Mode {
			/** @brief Values have not been set yet */
			POS_MODE_UNSET,
			/** @brief Node has been temporarily arranged like this, #
			 * since no explict position was available */
			POS_MODE_AUTO,
			/** @brief Node position has been explicity set */
			POS_MODE_SET,

		} mode = POS_MODE_UNSET;
		float x = 0;
		float y = 0;
	};

	struct NodeSize {
		/** @brief false: size-recording pending - true: sizes recorded */
		bool hasSize = false;
		/** @brief width of node */
		uint32_t width;
		/** @brief height of node */
		uint32_t height;
	};

public: // Public variables
	/** @brief Marks if node is has already been evalulated for current repositioning
	 *  @note Has to be set back to false after repositioning is done */
	bool reposInProgress = false;

private: // Internal variables
	/** @brief version, incremented on update */
	version_t version = 1;
	/** @brief Actual node management object (parent of this ElementDisplay-Object) */
	TreeManager::ElementNode* node;
	/** @brief true: node is collapsed, false: node is open */
	bool collapseNode = false;
	/** @brief position of the node */
	NodePosition nodePosition;
	/** @brief size of node (recorded in render) */
	NodeSize nodeSize;

public: // Public interface
	ElementDisplay(TreeManager::ElementNode* node);
	inline bool doCollapseNode() { return collapseNode; }
	void setCollapseNode(bool collapse);
	inline NodePosition getNodePosition() { return nodePosition; }
	void setNodePosition(NodePosition nodePosition);
	inline NodeSize getNodeSize() { return nodeSize; }
	inline bool hasNodeSize() { return nodeSize.hasSize; }
	void setNodeSize(uint32_t width, uint32_t height);
	inline version_t getVersion() { return version; }
};

} // namespace tstudio

#endif // SRC_STATE_ELEMENTDISPLAY_HPP_
