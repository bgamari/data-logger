#pragma once

/*
 * helper to iterate over measurables of all sensors
 *
 * next must be called after init before iterator is valid.
 */

struct measurable_iterator {
        struct sensor **sensor;
        int meas_idx;
};

static inline void
meas_iter_init(struct measurable_iterator *iter)
{
        iter->sensor = sensors;
        iter->meas_idx = -1;
}

static inline struct sensor*
meas_iter_get_sensor(struct measurable_iterator *iter)
{
        return *iter->sensor;
}

static inline struct measurable*
meas_iter_get_measurable(struct measurable_iterator *iter)
{
        struct sensor *s = meas_iter_get_sensor(iter);
        return &s->type->measurables[iter->meas_idx];
}

/* returns true if iterator is valid */
static inline bool
meas_iter_next(struct measurable_iterator *iter)
{
        struct sensor *s = meas_iter_get_sensor(iter);
        if (!s)
                return false;
        iter->meas_idx++;
        if (iter->meas_idx == s->type->n_measurables) {
                iter->meas_idx = 0;
                iter->sensor++;
        }
        if (!meas_iter_get_sensor(iter))
                return false;
        return true;
}
