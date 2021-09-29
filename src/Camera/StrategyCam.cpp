#include "StrategyCam.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>

void StrategyCamera::move_anchor( const glm::vec3& distance ){
	//origin -= distance;
	origin -= glm::vec3{
		distance.x * std::cos( -rotation ) + distance.z * std::sin( -rotation ),
		distance.y,
		distance.z * std::cos( -rotation ) - distance.x * std::sin( -rotation )};
}

void StrategyCamera::rotate_around_origin( float distance ){
	rotation -= distance;
}

void StrategyCamera::move_from_anchor( const glm::vec2& distance_height ){
	distance -= distance_height.x;
	height -= distance_height.y;

	height = std::max( min_height, height );
}

void StrategyCamera::set_proj( glm::mat4 projection ){
	proj = std::move( projection );
}

const glm::mat4 StrategyCamera::get_view() const {
	glm::vec3 pos{
		distance * std::cos( rotation ),
		height,
		distance * std::sin( rotation ),
	};

	return glm::lookAt( pos + origin, origin, { 0.0f, 1.0f, 0.0f });
}

const glm::mat4 StrategyCamera::get_proj() const {
	return proj;
}
