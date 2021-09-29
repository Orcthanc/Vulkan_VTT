#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct StrategyCamera {
	public:
		void move_anchor( const glm::vec3& distance );
		void rotate_around_origin( float distance );
		void move_from_anchor( const glm::vec2& distance_height );
		void set_proj( glm::mat4 projection );

		const glm::mat4 get_view() const;
		const glm::mat4 get_proj() const;

		float min_height{ 1 };

	private:
		glm::vec3 origin;
		float rotation{ 0 };
		float height{ 20 };
		float distance{ 20 };

		glm::mat4 proj;
};
