//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    Intersection inter = intersect(ray);
	if (!inter.happened) {
		return Vector3f(0.0, 0.0, 0.0);
	}

	if (inter.m->hasEmission()) {
		return inter.m->getEmission();
	}

	Vector3f L_indir, L_dir;
	Intersection light_pos;
	float pdf = 1.0f;
	sampleLight(light_pos, pdf);

	Vector3f p = inter.coords;
    Vector3f N = inter.normal.normalized();
	Vector3f wo = ray.direction;

    Vector3f x = light_pos.coords;
    Vector3f NN = (light_pos).normal.normalized();
    Vector3f emit = light_pos.emit;

    Vector3f ws = (x - p).normalized();
    float distance = (x - p).norm();

	Ray shadow_ray(p, ws);
	Intersection shadow_inter = intersect(shadow_ray);

	if (!shadow_inter.happened || shadow_inter.distance > (distance - 1e-3)) {
		Vector3f f_r = inter.m->eval(ray.direction, ws, N);

		float cos_theta = std::max(0.0f, dotProduct(ws, N));
        float cos_theta_x = std::max(0.0f, dotProduct(-ws, NN));

        L_dir = emit * f_r * cos_theta * cos_theta_x / (distance * distance) / pdf;
	}

	if (get_random_float() < RussianRoulette)
    {
        Vector3f wi = inter.m->sample(wo, N).normalized();
        Ray r(p, wi);
		Intersection inter2 = intersect(r);

        if (inter2.happened && !inter2.m->hasEmission()) {
            Vector3f eval = inter.m->eval(wo, wi, N);
			float pdf_0 = inter.m->pdf(wo, wi, N);
			float cos_theta = std::max(0.0f, dotProduct(wi ,N));
			L_indir = castRay(r, depth + 1) * eval * cos_theta / pdf_0 / RussianRoulette;
        }
    }

    return L_dir + L_indir;
}